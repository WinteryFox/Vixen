#include "FrameGraph.h"

#include <algorithm>
#include <stdexcept>
#include <type_traits>
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

        if (std::ranges::any_of(nodes, [&](const ResourceNode& resource) {
            return resource.name == name;
        }))
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

        return ResourceId{
            .index = index,
            .version = 0
        };
    }

    auto FrameGraph::Builder::compile() const -> std::expected<DependencyPlan, FrameGraphError> {
        const auto resourceError = [this](
            const FrameGraphErrorCode code,
            std::string message,
            uint32_t resourceIndex,
            const std::optional<uint32_t> version = std::nullopt
        ) {
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
        };

        DependencyPlan plan;

        plan.resources.reserve(nodes.size());

        for (std::size_t resourceIndex = 0; resourceIndex < nodes.size(); resourceIndex++) {
            const auto& node = nodes[resourceIndex];

            if (node.latestVersion == ResourceId::Invalid)
                return std::unexpected{
                    resourceError(
                        FrameGraphErrorCode::ResourceVersionOverflow,
                        "Resource '" + node.name + "' has exhausted its version range",
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
                                "Resource '" + node.name + "' has image type but description isn't that of an image",
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
                                "' has width/height/depth/layerCount/mipmapCount of 0 or empty usage flags",
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
                                    "' is imported but does not hold a valid image pointer",
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
                                    "Image resource '" + node.name +
                                    "' is imported but has missing or incorrectly typed initial or final state",
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
                                "' has buffer type but description isn't that of a buffer",
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
                                "' has a count/stride of 0 or empty usage flags",
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
                                    "' is imported but does not hold a valid buffer pointer",
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
                                    "Buffer resource '" + node.name +
                                    "' is imported but has missing or incorrectly typed initial or final state",
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
                            "Resource '" + node.name + "' has an unknown resource type",
                            resourceIndex
                        )
                    };
            }
        }

        plan.passes.resize(renderPasses.size());
        plan.executionOrder.reserve(renderPasses.size());

        for (uint32_t i = 0; i < renderPasses.size(); i++) {
            const auto& pass = renderPasses[i];

            for (const auto& usage : pass.getResourceUsages()) {
                const auto& result = std::visit([&](const auto& typedUsage) -> std::expected<void, FrameGraphError> {
                    using Usage = std::remove_cvref_t<decltype(typedUsage)>;

                    const auto& handle = typedUsage.input.isValid()
                                             ? typedUsage.input
                                             : typedUsage.output;
                    const auto& node = nodes[handle.id.index];

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
                            static_assert(std::is_same_v<Usage, BufferResourceUsage>);

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
                        return std::unexpected(std::move(error));
                    }

                    const VersionAccess access{
                        .pass = i,
                        .state = std::move(*state)
                    };

                    if (typedUsage.input.isValid()) {
                        auto& version = plan.resources[typedUsage.input.id.index][typedUsage.input.id.version];

                        version.consumers.push_back(access);
                    }

                    if (typedUsage.output.isValid()) {
                        auto& version = plan.resources[typedUsage.output.id.index][typedUsage.output.id.version];

                        if (version.producer.has_value())
                            return std::unexpected{
                                FrameGraphError{
                                    .code = FrameGraphErrorCode::DuplicateProducer,
                                    .message = "Pass '" + pass.getName() +
                                    "' has a duplicate producer for a resource"
                                }
                            };

                        version.producer = access;
                    }

                    return {};
                }, usage);

                if (!result)
                    return std::unexpected{result.error()};
            }
        }

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
