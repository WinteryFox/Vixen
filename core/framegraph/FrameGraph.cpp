#include "FrameGraph.h"

#include <cassert>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "FrameGraphError.h"
#include "ResourceStateMapping.h"
#include "buffer/Buffer.h"
#include "image/Image.h"

namespace Vixen {
    FrameGraph::FrameGraph(
        std::vector<ResourceNode>&& nodes,
        DependencyPlan&& dependencyPlan,
        FrameGraphResourceStorage&& storage,
        std::vector<RenderPass>&& renderPasses
    ) : nodes(std::move(nodes)),
        dependencyPlan(std::move(dependencyPlan)),
        storage(std::move(storage)),
        renderPasses(std::move(renderPasses)) {}

    FrameGraph::FrameGraph(FrameGraph&& other) noexcept = default;

    void FrameGraph::execute(RenderPassContext& context) {
        const auto resourceView = storage.getResources();

        for (const auto& pass : renderPasses) {
            // TODO: Execute pass
        }
    }

    ResourceId FrameGraph::Builder::addResource(
        std::string name,
        const ResourceType type,
        const ResourceLifetime lifetime,
        ResourceDescription description,
        ImportedResource importedResource,
        std::optional<ResourceState> initialState,
        std::optional<ResourceState> finalState
    ) {
        if (name.empty())
            throw std::invalid_argument{"Frame graph resource name must not be empty"};

        if (resourceNames.contains(name))
            throw std::invalid_argument{"A frame graph resource named '" + name + "' already exists"};

        if (nodes.size() >= ResourceId::Invalid)
            throw std::overflow_error{"Frame graph resource limit exceeded"};

        const bool imported = lifetime == ResourceLifetime::Imported;
        const bool hasImportedResource = !std::holds_alternative<std::monostate>(importedResource);
        if (imported != hasImportedResource ||
            imported != initialState.has_value() ||
            imported != finalState.has_value())
            throw std::logic_error{"Invalid frame graph resource ownership metadata"};

        if (imported) {
            const bool importedTypeMatches = type == ResourceType::Image
                                                 ? std::holds_alternative<Image*>(importedResource) &&
                                                 std::holds_alternative<ImageState>(*initialState) &&
                                                 std::holds_alternative<ImageState>(*finalState)
                                                 : std::holds_alternative<Buffer*>(importedResource) &&
                                                 std::holds_alternative<BufferState>(*initialState) &&
                                                 std::holds_alternative<BufferState>(*finalState);
            if (!importedTypeMatches)
                throw std::logic_error{"Imported frame graph resource metadata has the wrong type"};
        }

        const auto index = static_cast<uint32_t>(nodes.size());

        const auto [namePosition, inserted] = resourceNames.insert(name);
        assert(inserted);

        try {
            nodes.push_back({
                .name = std::move(name),
                .type = type,
                .lifetime = lifetime,
                .description = description,
                .latestVersion = 0,
                .importedResource = importedResource,
                .initialState = initialState,
                .finalState = finalState
            });
        } catch (...) {
            resourceNames.erase(namePosition);
            throw;
        }

        return ResourceId{
            .index = index,
            .version = 0
        };
    }

    FrameGraphError FrameGraph::Builder::resourceError(
        const FrameGraphErrorCode code,
        std::string message,
        const uint32_t resourceIndex,
        const std::optional<uint32_t> version
    ) const {
        return FrameGraphError{
            .code = code,
            .message = std::move(message),
            .passIndex = std::nullopt,
            .resourceIndex = resourceIndex,
            .resourceVersion = version,
            .passName = std::nullopt,
            .resourceName = nodes[resourceIndex].name,
            .details = {}
        };
    }

    FrameGraphError FrameGraph::Builder::passError(
        const FrameGraphErrorCode code,
        std::string message,
        const uint32_t passIndex,
        const std::optional<uint32_t> resourceIndex,
        const std::optional<uint32_t> resourceVersion
    ) const {
        return FrameGraphError{
            .code = code,
            .message = std::move(message),
            .passIndex = passIndex,
            .resourceIndex = resourceIndex,
            .resourceVersion = resourceVersion,
            .passName = renderPasses[passIndex].getName(),
            .resourceName = resourceIndex.has_value() ? std::optional{nodes[*resourceIndex].name} : std::nullopt,
            .details = {}
        };
    }

    auto FrameGraph::Builder::validateAndInitializeResources(DependencyPlan& plan) const
        -> std::expected<void, FrameGraphError> {
        for (std::size_t resourceIndex = 0; resourceIndex < nodes.size(); resourceIndex++) {
            const auto& node = nodes[resourceIndex];

            if (node.latestVersion == ResourceId::Invalid)
                return std::unexpected{
                    resourceError(
                        FrameGraphErrorCode::ResourceVersionOverflow,
                        "Resource '" + node.name +
                        "' has reached the reserved invalid version and cannot represent another version",
                        resourceIndex,
                        node.latestVersion
                    )
                };

            auto& versions = plan.resources.emplace_back(node.latestVersion + 1);

            versions[0].initializedExternally = node.lifetime == ResourceLifetime::Imported;

            switch (node.type) {
                case ResourceType::Image: {
                    if (!std::holds_alternative<ImageResourceDescription>(node.description))
                        return std::unexpected{
                            resourceError(
                                FrameGraphErrorCode::InvalidResourceDeclaration,
                                "Resource '" + node.name +
                                "' is declared as an image but does not contain an ImageResourceDescription",
                                resourceIndex
                            )
                        };

                    const auto& format = std::get<ImageResourceDescription>(node.description).format;

                    if (format.width == 0 ||
                        format.height == 0 ||
                        format.depth == 0 ||
                        format.layerCount == 0 ||
                        format.mipmapCount == 0 ||
                        format.usage.empty())
                        return std::unexpected{
                            resourceError(
                                FrameGraphErrorCode::InvalidResourceDeclaration,
                                "Image resource '" + node.name +
                                "' has an invalid description (width=" + std::to_string(format.width) +
                                ", height=" + std::to_string(format.height) +
                                ", depth=" + std::to_string(format.depth) +
                                ", layers=" + std::to_string(format.layerCount) +
                                ", mipmaps=" + std::to_string(format.mipmapCount) +
                                "); every dimension and count must be non-zero, and usage flags must not be empty",
                                resourceIndex
                            )
                        };

                    if (node.lifetime == ResourceLifetime::Imported) {
                        const auto image = std::get_if<Image*>(&node.importedResource);

                        if (!image || !*image)
                            return std::unexpected{
                                resourceError(
                                    FrameGraphErrorCode::InvalidResourceOwnership,
                                    "Image resource '" + node.name +
                                    "' is declared as imported but does not contain a non-null Image pointer",
                                    resourceIndex
                                )
                            };

                        if (!node.initialState.has_value() ||
                            !node.finalState.has_value() ||
                            !std::holds_alternative<ImageState>(*node.initialState) ||
                            !std::holds_alternative<ImageState>(*node.finalState))
                            return std::unexpected{
                                resourceError(
                                    FrameGraphErrorCode::InvalidResourceOwnership,
                                    "Imported image resource '" + node.name +
                                    "' must define both its initial and final states as ImageState values",
                                    resourceIndex
                                )
                            };
                    }

                    break;
                }

                case ResourceType::Buffer: {
                    if (!std::holds_alternative<BufferFormat>(node.description))
                        return std::unexpected{
                            resourceError(
                                FrameGraphErrorCode::InvalidResourceDeclaration,
                                "Resource '" + node.name +
                                "' is declared as a buffer but does not contain a BufferFormat",
                                resourceIndex
                            )
                        };

                    const auto& description = std::get<BufferFormat>(node.description);

                    if (description.count == 0 ||
                        description.stride == 0 ||
                        description.usage.empty())
                        return std::unexpected{
                            resourceError(
                                FrameGraphErrorCode::InvalidResourceDeclaration,
                                "Buffer resource '" + node.name +
                                "' has an invalid description (count=" + std::to_string(description.count) +
                                ", stride=" + std::to_string(description.stride) +
                                "); count and stride must be non-zero, and usage flags must not be empty",
                                resourceIndex
                            )
                        };

                    if (node.lifetime == ResourceLifetime::Imported) {
                        const auto buffer = std::get_if<Buffer*>(&node.importedResource);

                        if (!buffer || !*buffer)
                            return std::unexpected{
                                resourceError(
                                    FrameGraphErrorCode::InvalidResourceOwnership,
                                    "Buffer resource '" + node.name +
                                    "' is declared as imported but does not contain a non-null Buffer pointer",
                                    resourceIndex
                                )
                            };

                        if (!node.initialState.has_value() ||
                            !node.finalState.has_value() ||
                            !std::holds_alternative<BufferState>(*node.initialState) ||
                            !std::holds_alternative<BufferState>(*node.finalState))
                            return std::unexpected{
                                resourceError(
                                    FrameGraphErrorCode::InvalidResourceOwnership,
                                    "Imported buffer resource '" + node.name +
                                    "' must define both its initial and final states as BufferState values",
                                    resourceIndex
                                )
                            };
                    }

                    break;
                }

                default:
                    return std::unexpected{
                        resourceError(
                            FrameGraphErrorCode::InvalidResourceDeclaration,
                            "Resource '" + node.name + "' has an unrecognized ResourceType value",
                            resourceIndex
                        )
                    };
            }

            switch (node.lifetime) {
                case ResourceLifetime::Imported:
                    break;

                case ResourceLifetime::Persistent:
                case ResourceLifetime::Transient: {
                    if (!std::holds_alternative<std::monostate>(node.importedResource) ||
                        node.initialState.has_value() ||
                        node.finalState.has_value())
                        return std::unexpected{
                            resourceError(
                                FrameGraphErrorCode::InvalidResourceOwnership,
                                "Owned resource '" + node.name +
                                "' must not contain an imported object, an initial state, or a final state",
                                resourceIndex
                            )
                        };

                    break;
                }

                default:
                    return std::unexpected{
                        resourceError(
                            FrameGraphErrorCode::InvalidResourceOwnership,
                            "Resource '" + node.name + "' has an unrecognized ResourceLifetime value",
                            resourceIndex
                        )
                    };
            }
        }

        return {};
    }

    auto FrameGraph::Builder::recordPassUsages(
        DependencyPlan& plan,
        std::vector<uint32_t>& declaredVersions
    ) const -> std::expected<void, FrameGraphError> {
        for (uint32_t passIndex = 0; passIndex < renderPasses.size(); passIndex++) {
            const auto& pass = renderPasses[passIndex];

            for (const auto& usage : pass.getResourceUsages()) {
                auto result = std::visit(
                    [&]<typename T>(const T& typedUsage) -> std::expected<void, FrameGraphError> {
                        using Usage = std::remove_cvref_t<T>;

                        const auto hasInput = typedUsage.input.isValid();
                        const auto hasOutput = typedUsage.output.isValid();

                        switch (typedUsage.access) {
                            case ResourceAccess::Read:
                                if (!hasInput || hasOutput)
                                    return std::unexpected{
                                        passError(
                                            FrameGraphErrorCode::InvalidUsageShape,
                                            "Read usage must provide one valid input handle and no output handle",
                                            passIndex
                                        )
                                    };

                                break;

                            case ResourceAccess::Write:
                                if (hasInput || !hasOutput)
                                    return std::unexpected{
                                        passError(
                                            FrameGraphErrorCode::InvalidUsageShape,
                                            "Write usage must provide one valid output handle and no input handle",
                                            passIndex
                                        )
                                    };

                                break;

                            case ResourceAccess::ReadWrite: {
                                if (!hasInput || !hasOutput)
                                    return std::unexpected{
                                        passError(
                                            FrameGraphErrorCode::InvalidUsageShape,
                                            "Read-write usage must provide both a valid input handle and a valid output handle",
                                            passIndex
                                        )
                                    };

                                if (typedUsage.input.id.index != typedUsage.output.id.index)
                                    return std::unexpected{
                                        passError(
                                            FrameGraphErrorCode::InvalidUsageShape,
                                            "Read-write usage refers to different resources (input index " +
                                            std::to_string(typedUsage.input.id.index) + ", output index " +
                                            std::to_string(typedUsage.output.id.index) + ")",
                                            passIndex
                                        )
                                    };

                                if (typedUsage.input.id.version == std::numeric_limits<uint32_t>::max())
                                    return std::unexpected{
                                        passError(
                                            FrameGraphErrorCode::InvalidUsageShape,
                                            "Read-write usage cannot increment input version " +
                                            std::to_string(typedUsage.input.id.version) + " without overflowing",
                                            passIndex
                                        )
                                    };

                                if (typedUsage.output.id.version != typedUsage.input.id.version + 1)
                                    return std::unexpected{
                                        passError(
                                            FrameGraphErrorCode::InvalidUsageShape,
                                            "Read-write usage must produce the immediate successor of input version " +
                                            std::to_string(typedUsage.input.id.version) + "; received output version " +
                                            std::to_string(typedUsage.output.id.version),
                                            passIndex
                                        )
                                    };

                                break;
                            }

                            default:
                                return std::unexpected{
                                    passError(
                                        FrameGraphErrorCode::InvalidUsageShape,
                                        "Resource usage has an unrecognized ResourceAccess value",
                                        passIndex
                                    )
                                };
                        }

                        const auto handle = hasInput ? typedUsage.input : typedUsage.output;

                        if (handle.id.index >= nodes.size()) {
                            auto error = passError(
                                FrameGraphErrorCode::InvalidResourceHandle,
                                "Resource handle index " + std::to_string(handle.id.index) +
                                " is out of bounds for " + std::to_string(nodes.size()) + " resources",
                                passIndex
                            );

                            error.resourceIndex = handle.id.index;
                            error.resourceVersion = handle.id.version;

                            return std::unexpected{std::move(error)};
                        }

                        const auto& node = nodes[handle.id.index];

                        if (hasInput &&
                            typedUsage.input.id.version >= plan.resources[typedUsage.input.id.index].size())
                            return std::unexpected{
                                passError(
                                    FrameGraphErrorCode::InvalidResourceVersion,
                                    "Input handle requests version " +
                                    std::to_string(typedUsage.input.id.version) + " of resource '" + node.name +
                                    "', whose latest declared version is " +
                                    std::to_string(node.latestVersion),
                                    passIndex,
                                    typedUsage.input.id.index,
                                    typedUsage.input.id.version
                                )
                            };

                        if (hasOutput) {
                            if (typedUsage.output.id.version == 0)
                                return std::unexpected{
                                    passError(
                                        FrameGraphErrorCode::InvalidResourceVersion,
                                        "Output handle for resource '" + node.name +
                                        "' uses reserved version 0; outputs must produce version 1 or later",
                                        passIndex,
                                        typedUsage.output.id.index,
                                        typedUsage.output.id.version
                                    )
                                };

                            if (typedUsage.output.id.version >= plan.resources[typedUsage.output.id.index].size())
                                return std::unexpected{
                                    passError(
                                        FrameGraphErrorCode::InvalidResourceVersion,
                                        "Output handle requests version " +
                                        std::to_string(typedUsage.output.id.version) + " of resource '" + node.name +
                                        "', whose latest declared version is " +
                                        std::to_string(node.latestVersion),
                                        passIndex,
                                        typedUsage.output.id.index,
                                        typedUsage.output.id.version
                                    )
                                };
                        }

                        constexpr bool isImageUsage = std::is_same_v<Usage, ImageResourceUsage>;
                        constexpr bool isBufferUsage = std::is_same_v<Usage, BufferResourceUsage>;

                        static_assert(isImageUsage || isBufferUsage, "Unsupported frame graph resource usage type");

                        constexpr ResourceType expectedType = isImageUsage ? ResourceType::Image : ResourceType::Buffer;

                        if (node.type != expectedType)
                            return std::unexpected{
                                passError(
                                    FrameGraphErrorCode::ResourceTypeMismatch,
                                    "Resource '" + node.name + "' is used as " +
                                    (isImageUsage ? std::string{"an image"} : std::string{"a buffer"}) +
                                    " but is not declared with that resource type",
                                    passIndex,
                                    handle.id.index,
                                    handle.id.version
                                )
                            };

                        const uint32_t currentVersion = declaredVersions[handle.id.index];
                        if (hasOutput && currentVersion == std::numeric_limits<uint32_t>::max())
                            return std::unexpected{
                                passError(
                                    FrameGraphErrorCode::InvalidResourceVersion,
                                    "Resource '" + node.name +
                                    "' cannot produce a successor to version " +
                                    std::to_string(currentVersion) + " because the version number would overflow",
                                    passIndex,
                                    handle.id.index,
                                    currentVersion
                                )
                            };

                        bool versionSequenceMatches = false;
                        switch (typedUsage.access) {
                            case ResourceAccess::Read:
                                versionSequenceMatches = typedUsage.input.id.version == currentVersion;
                                break;

                            case ResourceAccess::Write:
                                versionSequenceMatches = typedUsage.output.id.version == currentVersion + 1;
                                break;

                            case ResourceAccess::ReadWrite:
                                versionSequenceMatches = typedUsage.input.id.version == currentVersion &&
                                    typedUsage.output.id.version == currentVersion + 1;
                                break;

                            default:
                                break;
                        }

                        if (!versionSequenceMatches) {
                            const uint32_t declaredVersion = hasInput
                                                                 ? typedUsage.input.id.version
                                                                 : typedUsage.output.id.version;
                            std::string message;

                            switch (typedUsage.access) {
                                case ResourceAccess::Read:
                                    message = "Read usage of resource '" + node.name + "' requests version " +
                                        std::to_string(typedUsage.input.id.version) +
                                        ", but its current version is " + std::to_string(currentVersion);
                                    break;

                                case ResourceAccess::Write:
                                    message = "Write usage of resource '" + node.name + "' produces version " +
                                        std::to_string(typedUsage.output.id.version) +
                                        ", but its next required version is " +
                                        std::to_string(currentVersion + 1);
                                    break;

                                case ResourceAccess::ReadWrite:
                                    message = "Read-write usage of resource '" + node.name + "' uses versions " +
                                        std::to_string(typedUsage.input.id.version) + " -> " +
                                        std::to_string(typedUsage.output.id.version) +
                                        ", but the required transition is " +
                                        std::to_string(currentVersion) + " -> " +
                                        std::to_string(currentVersion + 1);
                                    break;

                                default:
                                    break;
                            }

                            return std::unexpected{
                                passError(
                                    FrameGraphErrorCode::InvalidResourceVersion,
                                    std::move(message),
                                    passIndex,
                                    handle.id.index,
                                    declaredVersion
                                )
                            };
                        }

                        if (hasInput) {
                            const auto& inputVersion =
                                plan.resources[typedUsage.input.id.index][typedUsage.input.id.version];

                            if (!inputVersion.initializedExternally && !inputVersion.producer.has_value())
                                return std::unexpected{
                                    passError(
                                        FrameGraphErrorCode::UninitializedResourceRead,
                                        "Resource '" + node.name + "' version " +
                                        std::to_string(typedUsage.input.id.version) +
                                        " cannot be read because it is neither externally initialized nor produced by a pass",
                                        passIndex,
                                        typedUsage.input.id.index,
                                        typedUsage.input.id.version
                                    )
                                };
                        }

                        auto state = [&]() -> std::expected<ResourceState, FrameGraphError> {
                            if constexpr (std::is_same_v<Usage, ImageResourceUsage>) {
                                const auto& description = std::get<ImageResourceDescription>(node.description);

                                if (auto validation = validateImageUsage(description.format.usage, typedUsage.usage);
                                    !validation)
                                    return std::unexpected(validation.error());

                                auto mapped = mapImageResourceState(
                                    typedUsage.access,
                                    typedUsage.usage,
                                    typedUsage.stages
                                );
                                if (!mapped)
                                    return std::unexpected(mapped.error());

                                return ResourceState{*mapped};
                            } else {
                                const auto& description = std::get<BufferFormat>(node.description);

                                if (auto validation = validateBufferUsage(description.usage, typedUsage.usage);
                                    !validation)
                                    return std::unexpected(validation.error());

                                auto mapped = mapBufferResourceState(
                                    typedUsage.access,
                                    typedUsage.usage,
                                    typedUsage.stages
                                );
                                if (!mapped)
                                    return std::unexpected(mapped.error());

                                return ResourceState{*mapped};
                            }
                        }();

                        if (!state) {
                            auto error = std::move(state).error();

                            error.message = "Pass '" + pass.getName() + "' uses resource '" + node.name +
                                "': " + error.message;
                            error.passIndex = passIndex;
                            error.resourceIndex = handle.id.index;
                            error.resourceVersion = handle.id.version;
                            error.passName = pass.getName();
                            error.resourceName = node.name;

                            return std::unexpected(std::move(error));
                        }

                        const VersionAccess access{
                            .pass = passIndex,
                            .state = std::move(*state)
                        };

                        if (hasInput) {
                            auto& version = plan.resources[typedUsage.input.id.index][typedUsage.input.id.version];

                            version.consumers.push_back(access);
                        }

                        if (hasOutput) {
                            auto& version = plan.resources[typedUsage.output.id.index][typedUsage.output.id.version];

                            if (version.producer.has_value())
                                return std::unexpected{
                                    passError(
                                        FrameGraphErrorCode::DuplicateProducer,
                                        "Resource '" + node.name + "' version " +
                                        std::to_string(typedUsage.output.id.version) +
                                        " is already produced by pass '" +
                                        renderPasses[version.producer->pass].getName() +
                                        "'; each resource version may have only one producer",
                                        passIndex,
                                        typedUsage.output.id.index,
                                        typedUsage.output.id.version
                                    )
                                };

                            version.producer = access;
                            declaredVersions[typedUsage.output.id.index] = typedUsage.output.id.version;
                        }

                        return {};
                    }, usage);

                if (!result)
                    return std::unexpected{result.error()};
            }
        }

        return {};
    }

    auto FrameGraph::Builder::validateVersionTable(
        const DependencyPlan& plan,
        const std::vector<uint32_t>& declaredVersions
    ) const -> std::expected<void, FrameGraphError> {
        for (std::size_t resourceIndex = 0; resourceIndex < nodes.size(); resourceIndex++)
            if (declaredVersions[resourceIndex] != nodes[resourceIndex].latestVersion)
                return std::unexpected{
                    resourceError(
                        FrameGraphErrorCode::InvalidResourceVersion,
                        "Resource '" + nodes[resourceIndex].name +
                        "' reaches version " + std::to_string(declaredVersions[resourceIndex]) +
                        " during pass declaration, but advertises version " +
                        std::to_string(nodes[resourceIndex].latestVersion) + " as its latest version",
                        resourceIndex,
                        nodes[resourceIndex].latestVersion
                    )
                };

        for (std::size_t resourceIndex = 0; resourceIndex < plan.resources.size(); resourceIndex++) {
            const auto& node = nodes[resourceIndex];

            for (std::size_t version = 1; version <= node.latestVersion; version++)
                if (const auto& info = plan.resources[resourceIndex][version];
                    !info.producer.has_value())
                    return std::unexpected{
                        resourceError(
                            FrameGraphErrorCode::MissingProducer,
                            "Resource '" + node.name + "' version " + std::to_string(version) +
                            " has no producer; every non-zero resource version must be produced by exactly one pass",
                            resourceIndex,
                            version
                        )
                    };
        }

        return {};
    }

    void FrameGraph::Builder::buildDependencyEdges(DependencyPlan& plan) {
        struct DependencyKey {
            uint32_t predecessor;
            uint32_t successor;
            ResourceId handle;
            DependencyType type;

            bool operator==(const DependencyKey&) const = default;
        };

        struct DependencyKeyHash {
            std::size_t operator()(const DependencyKey& key) const noexcept {
                std::size_t result = 0;
                const auto combine = [&result](const uint32_t value) {
                    result ^= std::hash<uint32_t>{}(value) + 0x9e3779b9u +
                        (result << 6u) + (result >> 2u);
                };

                combine(key.predecessor);
                combine(key.successor);
                combine(key.handle.index);
                combine(key.handle.version);
                combine(static_cast<uint32_t>(key.type));
                return result;
            }
        };

        std::vector<std::unordered_map<uint32_t, std::size_t>> successorEdgeIndices(plan.passes.size());
        std::vector<std::unordered_map<uint32_t, std::size_t>> predecessorEdgeIndices(plan.passes.size());
        std::unordered_set<DependencyKey, DependencyKeyHash> insertedDependencies;

        const auto insertDependency = [&plan, &successorEdgeIndices, &predecessorEdgeIndices,
                &insertedDependencies](
            const uint32_t predecessor,
            const uint32_t successor,
            const Dependency& dependency
        ) {
            assert(predecessor < plan.passes.size());
            assert(successor < plan.passes.size());

            if (predecessor == successor)
                return;

            if (!insertedDependencies.insert({
                .predecessor = predecessor,
                .successor = successor,
                .handle = dependency.handle,
                .type = dependency.type
            }).second)
                return;

            const auto appendDependency = [&dependency](
                std::vector<PassEdge>& edges,
                std::unordered_map<uint32_t, std::size_t>& edgeIndices,
                const uint32_t adjacentPass
            ) {
                const auto [position, inserted] = edgeIndices.try_emplace(adjacentPass, edges.size());

                if (inserted)
                    edges.push_back({
                        .pass = adjacentPass,
                        .dependencies = {}
                    });

                edges[position->second].dependencies.push_back(dependency);
            };

            appendDependency(
                plan.passes[predecessor].successors,
                successorEdgeIndices[predecessor],
                successor
            );
            appendDependency(
                plan.passes[successor].predecessors,
                predecessorEdgeIndices[successor],
                predecessor
            );
        };

        constexpr auto writeAccesses = BarrierAccessBits::ShaderWrite |
            BarrierAccessBits::ColorAttachmentWrite |
            BarrierAccessBits::DepthStencilAttachmentWrite |
            BarrierAccessBits::CopyWrite |
            BarrierAccessBits::HostWrite |
            BarrierAccessBits::MemoryWrite |
            BarrierAccessBits::ResolveWrite |
            BarrierAccessBits::StorageClear;

        // TODO: Passes currently share RenderingDevice's graphics queue family, so queue ownership is
        //  compatible by construction. Queue-family identity must become part of this predicate
        //  when per-pass queue assignment is introduced.
        const auto readStatesCompatible = [writeAccesses](
            const ResourceState& left,
            const ResourceState& right
        ) {
            if (left.index() != right.index())
                return false;

            return std::visit(
                [&right, writeAccesses]<typename T>(const T& typedLeft) {
                    using State = std::remove_cvref_t<T>;
                    const auto& typedRight = std::get<State>(right);

                    const bool readOnly =
                        (typedLeft.access.value() & writeAccesses.value()) == 0 &&
                        (typedRight.access.value() & writeAccesses.value()) == 0;
                    if (!readOnly)
                        return false;

                    if constexpr (std::is_same_v<State, ImageState>)
                        return typedLeft.layout == typedRight.layout;
                    else
                        return true;
                },
                left
            );
        };

        for (uint32_t resourceIndex = 0; resourceIndex < plan.resources.size(); resourceIndex++) {
            const auto& versions = plan.resources[resourceIndex];

            for (uint32_t versionIndex = 0; versionIndex < versions.size(); versionIndex++) {
                const auto& version = versions[versionIndex];

                if (version.producer.has_value()) {
                    const Dependency readAfterWrite{
                        .handle = {
                            .index = resourceIndex,
                            .version = versionIndex
                        },
                        .type = DependencyType::ReadAfterWrite
                    };

                    for (const auto& consumer : version.consumers)
                        insertDependency(
                            version.producer->pass,
                            consumer.pass,
                            readAfterWrite
                        );
                }

                if (version.consumers.size() >= 2) {
                    const Dependency readAfterRead{
                        .handle = {
                            .index = resourceIndex,
                            .version = versionIndex
                        },
                        .type = DependencyType::ReadAfterRead
                    };

                    std::size_t previousGroupBegin = 0;
                    std::size_t currentGroupBegin = 0;

                    // Readers within a compatible group remain unordered. Every reader in the
                    // previous group gates every reader in the next group so an image layout
                    // transition cannot overlap a still-running reader.
                    for (std::size_t consumerIndex = 1; consumerIndex < version.consumers.size(); consumerIndex++) {
                        const auto& consumer = version.consumers[consumerIndex];

                        if (!readStatesCompatible(
                            version.consumers[currentGroupBegin].state,
                            consumer.state
                        )) {
                            previousGroupBegin = currentGroupBegin;
                            currentGroupBegin = consumerIndex;
                        }

                        if (currentGroupBegin == 0)
                            continue;

                        for (std::size_t previousIndex = previousGroupBegin;
                             previousIndex < currentGroupBegin;
                             previousIndex++)
                            insertDependency(
                                version.consumers[previousIndex].pass,
                                consumer.pass,
                                readAfterRead
                            );
                    }
                }

                if (versionIndex == 0)
                    continue;

                const auto& previousVersion = versions[versionIndex - 1];

                if (previousVersion.producer.has_value()) {
                    const Dependency writeAfterWrite{
                        .handle = {
                            .index = resourceIndex,
                            .version = versionIndex
                        },
                        .type = DependencyType::WriteAfterWrite
                    };

                    insertDependency(
                        previousVersion.producer->pass,
                        version.producer->pass,
                        writeAfterWrite
                    );
                }

                const Dependency writeAfterRead{
                    .handle = {
                        .index = resourceIndex,
                        .version = versionIndex
                    },
                    .type = DependencyType::WriteAfterRead
                };

                for (const auto& consumer : previousVersion.consumers)
                    insertDependency(
                        consumer.pass,
                        version.producer->pass,
                        writeAfterRead
                    );
            }
        }
    }

    auto FrameGraph::Builder::buildExecutionOrder(DependencyPlan& plan) const
        -> std::expected<void, FrameGraphError> {
        std::vector<std::size_t> inDegrees(plan.passes.size());
        std::priority_queue<uint32_t, std::vector<uint32_t>, std::greater<>> readyPasses;

        for (std::size_t passIndex = 0; passIndex < plan.passes.size(); passIndex++) {
            inDegrees[passIndex] = plan.passes[passIndex].predecessors.size();

            if (inDegrees[passIndex] == 0)
                readyPasses.push(static_cast<uint32_t>(passIndex));
        }

        while (!readyPasses.empty()) {
            const uint32_t passIndex = readyPasses.top();
            readyPasses.pop();

            plan.executionOrder.push_back(passIndex);

            for (const auto& successor : plan.passes[passIndex].successors) {
                assert(successor.pass < inDegrees.size());

                auto& successorInDegree = inDegrees[successor.pass];
                assert(successorInDegree > 0);

                successorInDegree--;
                if (successorInDegree == 0)
                    readyPasses.push(successor.pass);
            }
        }

        if (plan.executionOrder.size() != plan.passes.size()) {
            FrameGraphError error{
                .code = FrameGraphErrorCode::DependencyCycle,
                .message = "Frame graph contains a dependency cycle; scheduled " +
                std::to_string(plan.executionOrder.size()) + " of " +
                std::to_string(plan.passes.size()) + " passes",
                .passIndex = std::nullopt,
                .resourceIndex = std::nullopt,
                .resourceVersion = std::nullopt,
                .passName = std::nullopt,
                .resourceName = std::nullopt,
                .details = {}
            };

            for (std::size_t passIndex = 0; passIndex < inDegrees.size(); passIndex++) {
                if (inDegrees[passIndex] == 0)
                    continue;

                const auto unresolvedCount = inDegrees[passIndex];
                error.details.push_back(
                    "Pass '" + renderPasses[passIndex].getName() + "' remains blocked by " +
                    std::to_string(unresolvedCount) + " unresolved predecessor" +
                    (unresolvedCount == 1 ? "" : "s")
                );
            }

            return std::unexpected{std::move(error)};
        }

        return {};
    }

    auto FrameGraph::Builder::compile() const -> std::expected<DependencyPlan, FrameGraphError> {
        DependencyPlan plan;
        plan.resources.reserve(nodes.size());

        if (auto result = validateAndInitializeResources(plan); !result)
            return std::unexpected{std::move(result).error()};

        plan.passes.resize(renderPasses.size());
        plan.executionOrder.reserve(renderPasses.size());

        std::vector<uint32_t> declaredVersions(nodes.size(), 0);
        if (auto result = recordPassUsages(plan, declaredVersions); !result)
            return std::unexpected{std::move(result).error()};

        if (auto result = validateVersionTable(plan, declaredVersions); !result)
            return std::unexpected{std::move(result).error()};

        buildDependencyEdges(plan);

        if (auto result = buildExecutionOrder(plan); !result)
            return std::unexpected{std::move(result).error()};

        return plan;
    }

    ImageHandle FrameGraph::Builder::createImage(
        std::string name,
        ImageResourceDescription description,
        const ResourceLifetime lifetime
    ) {
        if (lifetime == ResourceLifetime::Imported)
            throw std::invalid_argument{"Imported images must be added through importImage"};

        if (description.format.width == 0 || description.format.height == 0 || description.format.depth == 0 ||
            description.format.layerCount == 0 || description.format.mipmapCount == 0)
            throw std::invalid_argument{"Frame graph image dimensions, layer count, and mipmap count must be nonzero"};

        if (description.format.usage.empty())
            throw std::invalid_argument{"Frame graph image usage must not be empty"};

        return ImageHandle{
            .id = addResource(
                std::move(name),
                ResourceType::Image,
                lifetime,
                description
            )
        };
    }

    BufferHandle FrameGraph::Builder::createBuffer(
        std::string name,
        BufferFormat description,
        const ResourceLifetime lifetime
    ) {
        if (lifetime == ResourceLifetime::Imported)
            throw std::invalid_argument{"Imported buffers must be added through importBuffer"};

        if (description.count == 0 || description.stride == 0)
            throw std::invalid_argument{"Frame graph buffer count and stride must be nonzero"};

        if (description.usage.empty())
            throw std::invalid_argument{"Frame graph buffer usage must not be empty"};

        return BufferHandle{
            .id = addResource(
                std::move(name),
                ResourceType::Buffer,
                lifetime,
                description
            )
        };
    }

    ImageHandle FrameGraph::Builder::importImage(
        std::string name,
        Image& image,
        ImageState initialState,
        ImageState finalState
    ) {
        return ImageHandle{
            .id = addResource(
                std::move(name),
                ResourceType::Image,
                ResourceLifetime::Imported,
                ImageResourceDescription{
                    .format = image.format,
                    .view = image.view
                },
                &image,
                initialState,
                finalState
            )
        };
    }

    BufferHandle FrameGraph::Builder::importBuffer(
        std::string name,
        Buffer& buffer,
        BufferState initialState,
        BufferState finalState
    ) {
        return BufferHandle{
            .id = addResource(
                std::move(name),
                ResourceType::Buffer,
                ResourceLifetime::Imported,
                BufferFormat{
                    .count = buffer.getCount(),
                    .stride = buffer.getStride(),
                    .usage = buffer.getUsage()
                },
                &buffer,
                initialState,
                finalState
            )
        };
    }

    auto FrameGraph::Builder::build(RenderingDevice& device) && -> std::expected<FrameGraph, FrameGraphError> {
        auto plan = compile();

        if (!plan)
            return std::unexpected{std::move(plan).error()};

        const std::size_t resourceCount = nodes.size();

        return FrameGraph{
            std::move(nodes),
            std::move(*plan),
            FrameGraphResourceStorage(device, resourceCount),
            std::move(renderPasses)
        };
    }
}
