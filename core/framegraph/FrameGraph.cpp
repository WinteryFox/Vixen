#include "FrameGraph.h"

#include <algorithm>
#include <cassert>
#include <format>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <experimental/scope>

#include "core/rendering/AttachmentInfo.h"
#include "FrameGraphError.h"
#include "FrameGraphPassResources.h"
#include "FrameGraphResourceAccess.h"
#include "core/rendering/RenderingDevice.h"
#include "core/rendering/RenderingDeviceDriver.h"
#include "RenderPassContext.h"
#include "ResourceStateMapping.h"
#include "core/buffer/Buffer.h"
#include "core/image/Image.h"

namespace Vixen {
    namespace {
        constexpr auto writeAccesses = BarrierAccessBits::ShaderWrite |
            BarrierAccessBits::ColorAttachmentWrite |
            BarrierAccessBits::DepthStencilAttachmentWrite |
            BarrierAccessBits::CopyWrite |
            BarrierAccessBits::HostWrite |
            BarrierAccessBits::MemoryWrite |
            BarrierAccessBits::ResolveWrite |
            BarrierAccessBits::StorageClear;

        [[nodiscard]]
        constexpr bool hasWriteAccess(const BarrierAccessFlags access) noexcept {
            return !(access & writeAccesses).empty();
        }

        [[nodiscard]]
        const char* resourceStateKind(const ResourceState& state) noexcept {
            if (std::holds_alternative<ImageState>(state))
                return "image";
            if (std::holds_alternative<BufferState>(state))
                return "buffer";

            return "valueless";
        }

        [[nodiscard]]
        constexpr const char* imageLayoutName(const ImageLayout layout) noexcept {
            switch (layout) {
                case ImageLayout::Undefined:
                    return "Undefined";
                case ImageLayout::General:
                    return "General";
                case ImageLayout::StorageOptimal:
                    return "StorageOptimal";
                case ImageLayout::ColorAttachmentOptimal:
                    return "ColorAttachmentOptimal";
                case ImageLayout::DepthStencilAttachmentOptimal:
                    return "DepthStencilAttachmentOptimal";
                case ImageLayout::DepthStencilReadOnlyOptimal:
                    return "DepthStencilReadOnlyOptimal";
                case ImageLayout::ShaderReadOnlyOptimal:
                    return "ShaderReadOnlyOptimal";
                case ImageLayout::CopySourceOptimal:
                    return "CopySourceOptimal";
                case ImageLayout::CopyDestinationOptimal:
                    return "CopyDestinationOptimal";
                case ImageLayout::ResolveSourceOptimal:
                    return "ResolveSourceOptimal";
                case ImageLayout::ResolveDestinationOptimal:
                    return "ResolveDestinationOptimal";
            }

            return "Unrecognized";
        }

        [[nodiscard]]
        std::string describeResourceState(const ResourceState& state) {
            if (state.valueless_by_exception())
                return "valueless resource state";

            return std::visit(
                []<typename State>(const State& typedState) {
                    if constexpr (std::is_same_v<State, ImageState>)
                        return std::format(
                            "image state (stages=0x{:08X}, access=0x{:08X}, layout={} [{}])",
                            typedState.stages.value(),
                            typedState.access.value(),
                            imageLayoutName(typedState.layout),
                            static_cast<std::underlying_type_t<ImageLayout>>(typedState.layout)
                        );
                    else {
                        static_assert(std::is_same_v<State, BufferState>);

                        return std::format(
                            "buffer state (stages=0x{:08X}, access=0x{:08X})",
                            typedState.stages.value(),
                            typedState.access.value()
                        );
                    }
                },
                state
            );
        }

        void appendStateDetails(
            FrameGraphError& error,
            const std::optional<ResourceState>& previous,
            const ResourceState& requested
        ) {
            error.details.emplace_back(
                previous.has_value()
                    ? "Previous/source " + describeResourceState(*previous)
                    : "Previous/source state: none"
            );
            error.details.emplace_back(
                "Requested/destination " + describeResourceState(requested)
            );
        }

        [[nodiscard]]
        auto invalidStateComparison(
            const ResourceState& source,
            const ResourceState& destination
        ) -> std::unexpected<FrameGraphError> {
            return std::unexpected{
                FrameGraphError{
                    .code = FrameGraphErrorCode::InvalidGraphInvariant,
                    .message = std::format(
                        "Cannot compare frame-graph resource states ('{}' and '{}'); both states must hold the same "
                        "valid resource-state kind for one logical resource",
                        resourceStateKind(source),
                        resourceStateKind(destination)
                    )
                }
            };
        }

        template <typename State>
        [[nodiscard]]
        constexpr bool typedStatesNeedBarrier(
            const State& source,
            const State& destination
        ) noexcept {
            if constexpr (std::is_same_v<State, ImageState>)
                if (source.layout != destination.layout)
                    return true;

            return hasWriteAccess(source.access) || hasWriteAccess(destination.access);
        }

        [[nodiscard]]
        auto needsBarrier(
            const ResourceState& source,
            const ResourceState& destination
        ) -> std::expected<bool, FrameGraphError> {
            if (source.valueless_by_exception() ||
                destination.valueless_by_exception() ||
                source.index() != destination.index())
                return invalidStateComparison(source, destination);

            return std::visit(
                [&destination]<typename State>(const State& typedSource) {
                    return typedStatesNeedBarrier(
                        typedSource,
                        std::get<State>(destination)
                    );
                },
                source
            );
        }

        // TODO: Passes currently share RenderingDevice's graphics queue family, so queue ownership is
        //  compatible by construction. Queue-family identity must become part of this predicate
        //  when per-pass queue assignment is introduced.
        [[nodiscard]]
        auto readStatesCompatible(
            const ResourceState& left,
            const ResourceState& right
        ) -> std::expected<bool, FrameGraphError> {
            auto barrierRequired = needsBarrier(left, right);
            if (!barrierRequired) {
                auto error = std::move(barrierRequired).error();
                appendStateDetails(error, left, right);
                return std::unexpected{std::move(error)};
            }

            return !*barrierRequired;
        }
    }

    FrameGraph::FrameGraph(
        RenderingDevice& device,
        std::vector<ResourceNode>&& nodes,
        DependencyPlan&& dependencyPlan,
        BarrierPlan&& barrierPlan,
        FrameGraphResourceStorage&& storage,
        std::vector<RenderPass>&& renderPasses,
        ExecutionPlan&& executionPlan
    ) : device(device),
        nodes(std::move(nodes)),
        dependencyPlan(std::move(dependencyPlan)),
        barrierPlan(std::move(barrierPlan)),
        storage(std::move(storage)),
        renderPasses(std::move(renderPasses)),
        executionPlan(std::move(executionPlan)),
        valid(true) {}

    FrameGraph::FrameGraph(FrameGraph&& other) noexcept
        : device(other.device),
          nodes(std::move(other.nodes)),
          dependencyPlan(std::move(other.dependencyPlan)),
          barrierPlan(std::move(other.barrierPlan)),
          storage(std::move(other.storage)),
          renderPasses(std::move(other.renderPasses)),
          executionPlan(std::move(other.executionPlan)),
          valid(std::exchange(other.valid, false)) {}

    auto FrameGraph::execute(
        CommandBuffer* commandBuffer
    ) -> std::expected<void, FrameGraphExecutionError> {
        if (!valid)
            return std::unexpected{
                FrameGraphExecutionError{
                    .code = FrameGraphExecutionErrorCode::MovedFromGraph,
                    .message = "Cannot execute a moved-from frame graph"
                }
            };

        if (!commandBuffer)
            return std::unexpected{
                FrameGraphExecutionError{
                    .code = FrameGraphExecutionErrorCode::InvalidCommandBuffer,
                    .message = "Given command buffer is a null pointer",
                }
            };

        const auto driver = device.getRenderingDeviceDriver();

        for (const auto& record : executionPlan.passes) {
            const uint32_t passIndex = record.passIndex;
            auto& pass = renderPasses[passIndex];

            FrameGraphPassResources passResources{
                storage.getResources(),
                record.permissions,
                nodes,
                passIndex,
                pass.getName(),
                record.sideEffecting,
                record.usesExternallySynchronizedResources
            };
            RenderPassContext context{
                *driver,
                commandBuffer,
                passResources
            };

            const bool shouldUseRenderPass = record.renderingInfo.has_value();

            driver->commandBeginLabel(commandBuffer, pass.getName(), record.debugLabelColor);

            auto labelGuard = std::experimental::scope_exit([
                &driver,
                commandBuffer
            ] {
                driver->commandEndLabel(commandBuffer);
            });

            emitBarrierBatches(*driver, commandBuffer, barrierPlan.beforePass[passIndex]);

            auto endRendering = [&driver, commandBuffer] {
                driver->commandEndRenderPass(commandBuffer);
            };
            using RenderingGuard = std::experimental::scope_exit<decltype(endRendering)>;
            std::optional<RenderingGuard> renderingGuard;
            if (shouldUseRenderPass) {
                driver->commandBeginRenderPass(commandBuffer, *record.renderingInfo);
                renderingGuard.emplace(endRendering);
            }

            try {
                pass.execute(context);
            } catch (...) {
                return std::unexpected{
                    FrameGraphExecutionError{
                        .code = FrameGraphExecutionErrorCode::CallbackFailed,
                        .message = std::format(
                            "An error occurred during frame-graph pass '{}' with index {} execution callback",
                            pass.getName(),
                            passIndex
                        ),
                        .passIndex = passIndex,
                        .passName = pass.getName(),
                        .cause = std::current_exception(),
                        .commandBufferMustBeDiscarded = true
                    }
                };
            }
        }

        emitBarrierBatches(*driver, commandBuffer, barrierPlan.finalBatches);

        return {};
    }

    FrameGraphError FrameGraph::Builder::allocationError(
        const FrameGraphErrorCode code,
        std::string message,
        uint32_t resourceIndex,
        std::optional<ResourceCreationError> cause
    ) const {
        return FrameGraphError{
            .code = code,
            .message = std::move(message),
            .passIndex = std::nullopt,
            .resourceIndex = resourceIndex,
            .resourceVersion = std::nullopt,
            .passName = std::nullopt,
            .resourceName = nodes[resourceIndex].name,
            .cause = std::move(cause),
            .details = {}
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
        constexpr auto hasAccessWithoutStages = [](const ResourceState& state) noexcept -> bool {
            return std::visit([](const auto& typedState) {
                return !typedState.access.empty() && typedState.stages.empty();
            }, state);
        };

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

                    if (const auto& format = std::get<ImageResourceDescription>(node.description).format;
                        format.width == 0 ||
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
                        if (const auto image = std::get_if<Image*>(&node.importedResource);
                            !image || !*image)
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

                        if (std::get<ImageState>(*node.finalState).layout == ImageLayout::Undefined)
                            return std::unexpected{
                                resourceError(
                                    FrameGraphErrorCode::InvalidResourceDeclaration,
                                    std::format(
                                        "Imported image resource '{}' must not have a final image layout of undefined",
                                        node.name
                                    ),
                                    resourceIndex
                                )
                            };

                        versions[0].initializedExternally = node.lifetime == ResourceLifetime::Imported &&
                            std::get<ImageState>(*node.initialState).layout != ImageLayout::Undefined;
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

                    if (description.size == 0 ||
                        description.usage.empty())
                        return std::unexpected{
                            resourceError(
                                FrameGraphErrorCode::InvalidResourceDeclaration,
                                "Buffer resource '" + node.name +
                                "' has an invalid description (size=" + std::to_string(description.size) +
                                "); size must be non-zero, and usage flags must not be empty",
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

                        versions[0].initializedExternally = true;
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
                    if (hasAccessWithoutStages(*node.initialState))
                        return std::unexpected{
                            resourceError(
                                FrameGraphErrorCode::InvalidResourceDeclaration,
                                std::format(
                                    "Imported resource '{}' has an invalid initial state: "
                                    "the access mask is non-empty but the pipeline stage mask is empty",
                                    node.name
                                ),
                                resourceIndex
                            )
                        };

                    if (hasAccessWithoutStages(*node.finalState))
                        return std::unexpected{
                            resourceError(
                                FrameGraphErrorCode::InvalidResourceDeclaration,
                                std::format(
                                    "Imported resource '{}' has an invalid final state: "
                                    "the access mask is non-empty but the pipeline stage mask is empty",
                                    node.name
                                ),
                                resourceIndex
                            )
                        };
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
            plan.passes[passIndex].resourceAccesses.reserve(pass.getResourceUsages().size());
            plan.passes[passIndex].permissions.reserve(pass.getResourceUsages().size());

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
                                    return std::unexpected(std::move(mapped).error());

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
                                    return std::unexpected(std::move(mapped).error());

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

                        plan.passes[passIndex].resourceAccesses.push_back({
                            .handle = handle.id,
                            .state = *state
                        });

                        auto attachmentRole = FrameGraphAttachmentRole::None;
                        if constexpr (isImageUsage) {
                            const auto colorAttachment = std::ranges::find_if(
                                pass.getColorAttachments(),
                                [&typedUsage](const RenderAttachment& attachment) {
                                    return typedUsage.output.isValid() &&
                                        attachment.handle == typedUsage.output;
                                }
                            );
                            if (colorAttachment != pass.getColorAttachments().end())
                                attachmentRole = FrameGraphAttachmentRole::Color;

                            if (const auto& depthStencil = pass.getDepthStencilAttachment();
                                depthStencil.has_value() &&
                                typedUsage.output.isValid() &&
                                depthStencil->handle == typedUsage.output) {
                                if (attachmentRole != FrameGraphAttachmentRole::None)
                                    return std::unexpected{
                                        passError(
                                            FrameGraphErrorCode::InvalidGraphInvariant,
                                            std::format(
                                                "Pass '{}' declares resource '{}' version {} as both a color and depth-stencil attachment",
                                                pass.getName(),
                                                node.name,
                                                typedUsage.output.id.version
                                            ),
                                            passIndex,
                                            typedUsage.output.id.index,
                                            typedUsage.output.id.version
                                        )
                                    };

                                attachmentRole = FrameGraphAttachmentRole::DepthStencil;
                            }
                        }

                        plan.passes[passIndex].permissions.push_back({
                            .input = hasInput ? typedUsage.input.id : ResourceId{},
                            .output = hasOutput ? typedUsage.output.id : ResourceId{},
                            .type = expectedType,
                            .access = typedUsage.access,
                            .usage = FrameGraphResourceUsageKind{typedUsage.usage},
                            .stages = typedUsage.stages,
                            .attachmentRole = attachmentRole
                        });

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

            auto& permissions = plan.passes[passIndex].permissions;
            std::ranges::sort(
                permissions,
                {},
                [](const FrameGraphResourcePermission& permission) {
                    return permission.input.isValid()
                               ? permission.input.index
                               : permission.output.index;
                }
            );

            for (std::size_t permissionIndex = 1;
                 permissionIndex < permissions.size();
                 permissionIndex++) {
                const auto previousResourceIndex = permissions[permissionIndex - 1].input.isValid()
                                                       ? permissions[permissionIndex - 1].input.index
                                                       : permissions[permissionIndex - 1].output.index;
                const auto resourceIndex = permissions[permissionIndex].input.isValid()
                                               ? permissions[permissionIndex].input.index
                                               : permissions[permissionIndex].output.index;

                if (previousResourceIndex == resourceIndex)
                    return std::unexpected{
                        passError(
                            FrameGraphErrorCode::InvalidGraphInvariant,
                            std::format(
                                "Pass '{}' compiled more than one permission for resource '{}' (index {}); each pass may declare a logical resource only once",
                                pass.getName(),
                                nodes[resourceIndex].name,
                                resourceIndex
                            ),
                            passIndex,
                            resourceIndex
                        )
                    };
            }
        }

        return {};
    }

    auto FrameGraph::Builder::validatePassStages() const -> std::expected<void, FrameGraphError> {
        constexpr auto graphicsOnlyStages = PipelineStageBits::VertexInput |
            PipelineStageBits::VertexShader |
            PipelineStageBits::TessellationControl |
            PipelineStageBits::TessellationEvaluation |
            PipelineStageBits::GeometryShader |
            PipelineStageBits::FragmentShader |
            PipelineStageBits::EarlyFragmentTests |
            PipelineStageBits::LateFragmentTests |
            PipelineStageBits::ColorAttachmentOutput |
            PipelineStageBits::AllGraphics;

        for (size_t passIndex = 0; passIndex < renderPasses.size(); passIndex++) {
            const auto& pass = renderPasses[passIndex];
            const auto passType = pass.getType();

            if (passType != RenderPassType::Graphics && passType != RenderPassType::Compute)
                return std::unexpected{
                    passError(
                        FrameGraphErrorCode::InvalidGraphInvariant,
                        std::format(
                            "Pass '{}' has an unrecognized RenderPassType value ({})",
                            pass.getName(),
                            static_cast<std::underlying_type_t<RenderPassType>>(passType)
                        ),
                        passIndex
                    )
                };

            const bool graphicsPass = passType == RenderPassType::Graphics;

            for (const auto& usage : pass.getResourceUsages()) {
                if (const auto res = std::visit([&](const auto& u) -> std::expected<void, FrameGraphError> {
                    const auto handle = u.input.isValid() ? u.input : u.output;
                    const auto& node = nodes[handle.id.index];

                    if (graphicsPass && u.stages.contains(PipelineStageBits::ComputeShader))
                        return std::unexpected{
                            passError(
                                FrameGraphErrorCode::IncompatiblePassStages,
                                std::format(
                                    "Graphics pass '{}' declares compute pipeline stage for resource '{}'",
                                    pass.getName(),
                                    node.name
                                ),
                                passIndex,
                                handle.id.index,
                                handle.id.version
                            )
                        };

                    if (!graphicsPass && !(u.stages & graphicsOnlyStages).empty())
                        return std::unexpected{
                            passError(
                                FrameGraphErrorCode::IncompatiblePassStages,
                                std::format(
                                    "Compute pass '{}' declares graphics-only pipeline stages for resource '{}'",
                                    pass.getName(),
                                    node.name
                                ),
                                passIndex,
                                handle.id.index,
                                handle.id.version
                            )
                        };

                    return {};
                }, usage); !res)
                    return res;
            }
        }

        return {};
    }

    auto FrameGraph::Builder::validateAttachments(
        const DependencyPlan& plan
    ) const -> std::expected<void, FrameGraphError> {
        struct AttachmentShape {
            uint32_t width;
            uint32_t height;
            uint32_t layerCount;
            ImageSamples samples;
            std::string attachmentName;
        };

        const auto sampleDescription = [](const ImageSamples samples) {
            switch (samples) {
                case ImageSamples::One:
                    return "1 sample";
                case ImageSamples::Two:
                    return "2 samples";
                case ImageSamples::Four:
                    return "4 samples";
                case ImageSamples::Eight:
                    return "8 samples";
                case ImageSamples::Sixteen:
                    return "16 samples";
                case ImageSamples::ThirtyTwo:
                    return "32 samples";
                case ImageSamples::SixtyFour:
                    return "64 samples";
                default:
                    return "an unrecognized sample count";
            }
        };

        for (uint32_t passIndex = 0; passIndex < renderPasses.size(); passIndex++) {
            const auto& pass = renderPasses[passIndex];
            std::optional<AttachmentShape> referenceShape;

            const auto validateAttachment = [&](
                const RenderAttachment& attachment,
                const std::string& attachmentName,
                const ImageUsageBits expectedUsage,
                const char* expectedUsageName,
                const FrameGraphAttachmentRole expectedRole
            ) -> std::expected<void, FrameGraphError> {
                const auto handle = attachment.handle;
                const auto hasKnownResource = handle.isValid() && handle.id.index < nodes.size();
                const auto resourceIndex = hasKnownResource
                                               ? std::optional{handle.id.index}
                                               : std::nullopt;
                const auto resourceVersion = handle.isValid()
                                                 ? std::optional{handle.id.version}
                                                 : std::nullopt;
                const auto handleDescription = !handle.isValid()
                                                   ? "an invalid image handle"
                                                   : hasKnownResource
                                                   ? "resource '" + nodes[handle.id.index].name +
                                                   "' (index " + std::to_string(handle.id.index) +
                                                   ", version " + std::to_string(handle.id.version) + ")"
                                                   : "resource index " + std::to_string(handle.id.index) +
                                                   " version " + std::to_string(handle.id.version);

                ResourceAccess expectedAccess;
                const char* expectedAccessName;

                switch (attachment.loadAction) {
                    case LoadAction::Load:
                        expectedAccess = ResourceAccess::ReadWrite;
                        expectedAccessName = "ResourceAccess::ReadWrite";
                        break;

                    case LoadAction::Clear:
                    case LoadAction::DontCare:
                        expectedAccess = ResourceAccess::Write;
                        expectedAccessName = "ResourceAccess::Write";
                        break;

                    default:
                        return std::unexpected{
                            passError(
                                FrameGraphErrorCode::InvalidAttachment,
                                std::format(
                                    "{} in pass '{}' has an unrecognized load action",
                                    attachmentName,
                                    pass.getName()
                                ),
                                passIndex,
                                resourceIndex,
                                resourceVersion
                            )
                        };
                }

                const auto authorization = authorizeFrameGraphResourceAccess(
                    plan.passes[passIndex].permissions,
                    FrameGraphResourceAccessRequest{
                        .handle = handle.id,
                        .type = ResourceType::Image,
                        .access = expectedAccess,
                        .usage = FrameGraphResourceUsageKind{expectedUsage},
                        .attachmentRole = expectedRole
                    },
                    FrameGraphResourceAccessContext{
                        .passIndex = passIndex,
                        .passName = pass.getName(),
                        .resourceName = hasKnownResource
                                            ? std::string_view{nodes[handle.id.index].name}
                                            : std::string_view{},
                        .sideEffecting = pass.isSideEffecting(),
                        .usesExternallySynchronizedResources = pass.usesExternalResources()
                    }
                );
                if (!authorization) {
                    auto accessError = std::move(authorization).error();
                    auto error = passError(
                        FrameGraphErrorCode::InvalidAttachment,
                        std::format(
                            "{} in pass '{}' cannot authorize {} as {} with {}: {}",
                            attachmentName,
                            pass.getName(),
                            handleDescription,
                            expectedUsageName,
                            expectedAccessName,
                            accessError.message
                        ),
                        passIndex,
                        resourceIndex,
                        resourceVersion
                    );
                    error.details = std::move(accessError.details);

                    return std::unexpected{
                        std::move(error)
                    };
                }

                const auto& description = std::get<ImageResourceDescription>(nodes[handle.id.index].description);
                const auto aspects = getImageAspects(description.view.format);

                const bool expectsColor = expectedUsage == ImageUsageBits::ColorAttachment;
                const bool hasCompatibleAspect = expectsColor
                                                     ? aspects.contains(ImageAspectBits::Color)
                                                     : aspects.contains(ImageAspectBits::Depth) ||
                                                     aspects.contains(ImageAspectBits::Stencil);

                if (!hasCompatibleAspect) {
                    const auto expectedAspect = expectsColor ? "a color aspect" : "a depth or stencil aspect";

                    return std::unexpected{
                        passError(
                            FrameGraphErrorCode::InvalidAttachment,
                            std::format(
                                "{} in pass '{}' references {}, but its image view format does not provide {}",
                                attachmentName,
                                pass.getName(),
                                handleDescription,
                                expectedAspect
                            ),
                            passIndex,
                            resourceIndex,
                            resourceVersion
                        )
                    };
                }

                const auto hasMatchingProducer = handle.id.index < plan.resources.size() &&
                    handle.id.version < plan.resources[handle.id.index].size() &&
                    plan.resources[handle.id.index][handle.id.version].producer.has_value() &&
                    plan.resources[handle.id.index][handle.id.version].producer->pass == passIndex;

                if (!hasMatchingProducer)
                    return std::unexpected{
                        passError(
                            FrameGraphErrorCode::InvalidAttachment,
                            std::format(
                                "{} in pass '{}' references {}, but that version is not produced by this pass",
                                attachmentName,
                                pass.getName(),
                                handleDescription
                            ),
                            passIndex,
                            resourceIndex,
                            resourceVersion
                        )
                    };

                const auto& format = description.format;

                if (!referenceShape.has_value()) {
                    referenceShape = AttachmentShape{
                        .width = format.width,
                        .height = format.height,
                        .layerCount = format.layerCount,
                        .samples = format.samples,
                        .attachmentName = attachmentName
                    };

                    return {};
                }

                if (format.width != referenceShape->width || format.height != referenceShape->height)
                    return std::unexpected{
                        passError(
                            FrameGraphErrorCode::InvalidAttachment,
                            std::format(
                                "{} in pass '{}' references {} with extent {}x{}, but {} uses extent {}x{}; "
                                "all attachments in a pass must have matching extents",
                                attachmentName,
                                pass.getName(),
                                handleDescription,
                                std::to_string(format.width),
                                std::to_string(format.height),
                                referenceShape->attachmentName,
                                std::to_string(referenceShape->width),
                                std::to_string(referenceShape->height)
                            ),
                            passIndex,
                            resourceIndex,
                            resourceVersion
                        )
                    };

                if (format.layerCount != referenceShape->layerCount)
                    return std::unexpected{
                        passError(
                            FrameGraphErrorCode::InvalidAttachment,
                            std::format(
                                "{} in pass '{}' references {} with {} layers, but {} uses {} layers; "
                                "all attachments in a pass must have matching layer counts",
                                attachmentName,
                                pass.getName(),
                                handleDescription,
                                std::to_string(format.layerCount),
                                referenceShape->attachmentName,
                                std::to_string(referenceShape->layerCount)
                            ),
                            passIndex,
                            resourceIndex,
                            resourceVersion
                        )
                    };

                if (format.samples != referenceShape->samples)
                    return std::unexpected{
                        passError(
                            FrameGraphErrorCode::InvalidAttachment,
                            std::format(
                                "{} in pass '{}' references {} using {}, but {} uses {}; "
                                "all attachments in a pass must have matching sample counts",
                                attachmentName,
                                pass.getName(),
                                handleDescription,
                                sampleDescription(format.samples),
                                referenceShape->attachmentName,
                                sampleDescription(referenceShape->samples)
                            ),
                            passIndex,
                            resourceIndex,
                            resourceVersion
                        )
                    };

                return {};
            };

            for (std::size_t attachmentIndex = 0;
                 attachmentIndex < pass.getColorAttachments().size();
                 attachmentIndex++) {
                if (auto result = validateAttachment(
                        pass.getColorAttachments()[attachmentIndex],
                        "Color attachment " + std::to_string(attachmentIndex),
                        ImageUsageBits::ColorAttachment,
                        "ImageUsageBits::ColorAttachment",
                        FrameGraphAttachmentRole::Color
                    );
                    !result)
                    return std::unexpected{std::move(result).error()};
            }

            if (const auto& attachment = pass.getDepthStencilAttachment(); attachment.has_value())
                if (auto result = validateAttachment(
                        *attachment,
                        "Depth-stencil attachment",
                        ImageUsageBits::DepthStencilAttachment,
                        "ImageUsageBits::DepthStencilAttachment",
                        FrameGraphAttachmentRole::DepthStencil
                    );
                    !result)
                    return std::unexpected{std::move(result).error()};
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

    auto FrameGraph::Builder::buildDependencyEdges(
        DependencyPlan& plan
    ) const -> std::expected<void, FrameGraphError> {
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

                        auto compatible = readStatesCompatible(
                            version.consumers[currentGroupBegin].state,
                            consumer.state
                        );
                        if (!compatible) {
                            auto error = std::move(compatible).error();
                            error.resourceIndex = resourceIndex;
                            error.resourceVersion = versionIndex;
                            error.resourceName = nodes[resourceIndex].name;
                            return std::unexpected{std::move(error)};
                        }

                        if (!*compatible) {
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

        return {};
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

    auto FrameGraph::Builder::buildInitialTrackedStates() const ->
        std::expected<std::vector<TrackedResourceState>, FrameGraphError> {
        std::vector<TrackedResourceState> states;
        states.reserve(nodes.size());

        for (std::size_t resourceIndex = 0; resourceIndex < nodes.size(); resourceIndex++) {
            const auto& node = nodes[resourceIndex];
            TrackedResourceState trackedState{
                .state = std::nullopt,
                .lastHandle = {
                    .index = static_cast<uint32_t>(resourceIndex),
                    .version = 0
                },
                .used = false
            };

            switch (node.lifetime) {
                case ResourceLifetime::Transient:
                case ResourceLifetime::Persistent: {
                    switch (node.type) {
                        case ResourceType::Image:
                            trackedState.state = ResourceState{
                                ImageState{
                                    .stages = {},
                                    .access = {},
                                    .layout = ImageLayout::Undefined
                                }
                            };
                            break;

                        case ResourceType::Buffer:
                            break;

                        default:
                            return std::unexpected{
                                resourceError(
                                    FrameGraphErrorCode::InvalidGraphInvariant,
                                    "Cannot initialize tracked state for owned resource '" + node.name +
                                    "': the validated resource has an unrecognized ResourceType value",
                                    static_cast<uint32_t>(resourceIndex)
                                )
                            };
                    }

                    break;
                }

                case ResourceLifetime::Imported: {
                    if (!node.initialState.has_value())
                        return std::unexpected{
                            resourceError(
                                FrameGraphErrorCode::InvalidGraphInvariant,
                                "Cannot initialize tracked state for imported resource '" + node.name +
                                "': the validated resource has no initial state",
                                static_cast<uint32_t>(resourceIndex)
                            )
                        };

                    const char* expectedState;
                    bool stateTypeMatches;
                    switch (node.type) {
                        case ResourceType::Image:
                            expectedState = "image";
                            stateTypeMatches = std::holds_alternative<ImageState>(*node.initialState);
                            break;

                        case ResourceType::Buffer:
                            expectedState = "buffer";
                            stateTypeMatches = std::holds_alternative<BufferState>(*node.initialState);
                            break;

                        default:
                            return std::unexpected{
                                resourceError(
                                    FrameGraphErrorCode::InvalidGraphInvariant,
                                    "Cannot initialize tracked state for imported resource '" + node.name +
                                    "': the validated resource has an unrecognized ResourceType value",
                                    static_cast<uint32_t>(resourceIndex)
                                )
                            };
                    }

                    if (!stateTypeMatches)
                        return std::unexpected{
                            resourceError(
                                FrameGraphErrorCode::InvalidGraphInvariant,
                                "Cannot initialize tracked state for imported resource '" + node.name +
                                "': expected a valid " + expectedState + " state but found a " +
                                resourceStateKind(*node.initialState) + " state",
                                static_cast<uint32_t>(resourceIndex)
                            )
                        };

                    trackedState.state = *node.initialState;
                    break;
                }

                default:
                    return std::unexpected{
                        resourceError(
                            FrameGraphErrorCode::InvalidGraphInvariant,
                            "Cannot initialize tracked state for resource '" + node.name +
                            "': the validated resource has an unrecognized ResourceLifetime value",
                            static_cast<uint32_t>(resourceIndex)
                        )
                    };
            }

            states.push_back(trackedState);
        }

        return states;
    }

    auto FrameGraph::Builder::planResourceAccess(
        TrackedResourceState& tracker,
        const PassDependencies::CompiledResourceAccess& access,
        std::vector<Transition>& transitions
    ) const -> std::expected<void, FrameGraphError> {
        if (!access.handle.isValid())
            return std::unexpected{
                FrameGraphError{
                    .code = FrameGraphErrorCode::InvalidGraphInvariant,
                    .message = std::format(
                        "Compiled resource access contains an invalid handle (index={}, version={})",
                        access.handle.index,
                        access.handle.version
                    ),
                    .resourceVersion = access.handle.version
                }
            };

        if (access.handle.index >= nodes.size())
            return std::unexpected{
                FrameGraphError{
                    .code = FrameGraphErrorCode::InvalidGraphInvariant,
                    .message = std::format(
                        "Compiled resource access references resource index {}, but the graph contains {} resources",
                        access.handle.index,
                        nodes.size()
                    ),
                    .resourceIndex = access.handle.index,
                    .resourceVersion = access.handle.version
                }
            };

        const auto& node = nodes[access.handle.index];

        if (access.handle.version > node.latestVersion)
            return std::unexpected{
                resourceError(
                    FrameGraphErrorCode::InvalidGraphInvariant,
                    std::format(
                        "Compiled access to resource '{}' references version {}, but its latest declared version is {}",
                        node.name,
                        access.handle.version,
                        node.latestVersion
                    ),
                    access.handle.index,
                    access.handle.version
                )
            };

        if (tracker.lastHandle.index != access.handle.index)
            return std::unexpected{
                resourceError(
                    FrameGraphErrorCode::InvalidGraphInvariant,
                    std::format(
                        "Tracked state for resource '{}' belongs to resource index {}, but it was asked to process "
                        "an access to resource index {}",
                        node.name,
                        tracker.lastHandle.index,
                        access.handle.index
                    ),
                    access.handle.index,
                    access.handle.version
                )
            };

        switch (node.type) {
            case ResourceType::Image:
                if (!std::holds_alternative<ImageResourceDescription>(node.description))
                    return std::unexpected{
                        resourceError(
                            FrameGraphErrorCode::InvalidGraphInvariant,
                            "Cannot plan access to image resource '" + node.name +
                            "': the validated node does not contain an ImageResourceDescription",
                            access.handle.index,
                            access.handle.version
                        )
                    };

                if (!std::holds_alternative<ImageState>(access.state))
                    return std::unexpected{
                        resourceError(
                            FrameGraphErrorCode::InvalidGraphInvariant,
                            "Cannot plan access to image resource '" + node.name +
                            "': its compiled access contains a " + resourceStateKind(access.state) + " state",
                            access.handle.index,
                            access.handle.version
                        )
                    };

                if (tracker.state.has_value() &&
                    !std::holds_alternative<ImageState>(*tracker.state))
                    return std::unexpected{
                        resourceError(
                            FrameGraphErrorCode::InvalidGraphInvariant,
                            "Cannot plan access to image resource '" + node.name +
                            "': its tracker contains a " + resourceStateKind(*tracker.state) + " state",
                            access.handle.index,
                            access.handle.version
                        )
                    };

                break;

            case ResourceType::Buffer:
                if (!std::holds_alternative<BufferFormat>(node.description))
                    return std::unexpected{
                        resourceError(
                            FrameGraphErrorCode::InvalidGraphInvariant,
                            "Cannot plan access to buffer resource '" + node.name +
                            "': the validated node does not contain a BufferFormat",
                            access.handle.index,
                            access.handle.version
                        )
                    };

                if (!std::holds_alternative<BufferState>(access.state))
                    return std::unexpected{
                        resourceError(
                            FrameGraphErrorCode::InvalidGraphInvariant,
                            "Cannot plan access to buffer resource '" + node.name +
                            "': its compiled access contains an " + resourceStateKind(access.state) + " state",
                            access.handle.index,
                            access.handle.version
                        )
                    };

                if (tracker.state.has_value() &&
                    !std::holds_alternative<BufferState>(*tracker.state))
                    return std::unexpected{
                        resourceError(
                            FrameGraphErrorCode::InvalidGraphInvariant,
                            "Cannot plan access to buffer resource '" + node.name +
                            "': its tracker contains an " + resourceStateKind(*tracker.state) + " state",
                            access.handle.index,
                            access.handle.version
                        )
                    };

                break;

            default:
                return std::unexpected{
                    resourceError(
                        FrameGraphErrorCode::InvalidGraphInvariant,
                        "Cannot plan access to resource '" + node.name +
                        "': the validated node has an unrecognized ResourceType value",
                        access.handle.index,
                        access.handle.version
                    )
                };
        }

        if (!tracker.state) {
            if (node.type != ResourceType::Buffer)
                return std::unexpected{
                    resourceError(
                        FrameGraphErrorCode::InvalidGraphInvariant,
                        "Cannot plan access to image resource '" + node.name +
                        "': its tracker has no state; only a newly created buffer may lack an initial state",
                        access.handle.index,
                        access.handle.version
                    )
                };

            tracker.state = access.state;
            tracker.lastHandle = access.handle;

            return {};
        }

        auto isBarrierRequired = needsBarrier(*tracker.state, access.state);
        if (!isBarrierRequired) {
            auto error = std::move(isBarrierRequired).error();
            error.resourceIndex = access.handle.index;
            error.resourceVersion = access.handle.version;
            error.resourceName = nodes[access.handle.index].name;

            return std::unexpected{std::move(error)};
        }

        if (!*isBarrierRequired) {
            std::visit(
                [&access]<typename State>(State& current) {
                    const auto& desired = std::get<State>(access.state);

                    current.stages |= desired.stages;
                    current.access |= desired.access;
                },
                *tracker.state
            );

            tracker.lastHandle = access.handle;

            return {};
        }

        std::visit(
            [&]<typename State>(State& current) {
                if constexpr (std::is_same_v<State, ImageState>) {
                    const auto& description = std::get<ImageResourceDescription>(
                        nodes[access.handle.index].description);

                    transitions.emplace_back(ImageTransition{
                        .handle = ImageHandle{
                            .id = {
                                .index = access.handle.index,
                                .version = access.handle.version
                            }
                        },
                        .source = current,
                        .destination = std::get<ImageState>(access.state),
                        .subresources = {
                            .aspect = getImageAspects(description.format.format),
                            .baseMipmap = 0,
                            .mipmapCount = description.format.mipmapCount,
                            .baseLayer = 0,
                            .layerCount = description.format.layerCount
                        }
                    });
                } else {
                    static_assert(std::is_same_v<State, BufferState>);

                    const auto& description = std::get<BufferFormat>(nodes[access.handle.index].description);

                    transitions.emplace_back(BufferTransition{
                        .handle = BufferHandle{
                            .id = {
                                .index = access.handle.index,
                                .version = access.handle.version
                            }
                        },
                        .source = current,
                        .destination = std::get<BufferState>(access.state),
                        .offset = 0,
                        .size = description.size
                    });
                }
            },
            *tracker.state
        );

        tracker.state = access.state;
        tracker.lastHandle = access.handle;

        return {};
    }

    auto FrameGraph::Builder::buildTransitionPlan(DependencyPlan& plan) const -> std::expected<void, FrameGraphError> {
        if (plan.executionOrder.size() != plan.passes.size() ||
            plan.passes.size() != renderPasses.size())
            return std::unexpected{
                FrameGraphError{
                    .code = FrameGraphErrorCode::InvalidGraphInvariant,
                    .message = std::format(
                        "Transition planning requires one execution-order entry and one compiled pass record per "
                        "declared render pass, but found {} execution-order entries, {} compiled pass records, "
                        "and {} render passes",
                        plan.executionOrder.size(),
                        plan.passes.size(),
                        renderPasses.size()
                    )
                }
            };

        std::vector<bool> visitedPasses(renderPasses.size(), false);
        for (const uint32_t passIndex : plan.executionOrder) {
            if (passIndex >= plan.passes.size() || passIndex >= renderPasses.size())
                return std::unexpected{
                    FrameGraphError{
                        .code = FrameGraphErrorCode::InvalidGraphInvariant,
                        .message = std::format(
                            "Transition planning cannot process execution-order pass index {}: the dependency plan "
                            "contains {} compiled passes and the builder contains {} render passes",
                            passIndex,
                            plan.passes.size(),
                            renderPasses.size()
                        ),
                        .passIndex = passIndex
                    }
                };

            if (visitedPasses[passIndex])
                return std::unexpected{
                    FrameGraphError{
                        .code = FrameGraphErrorCode::InvalidGraphInvariant,
                        .message = std::format(
                            "Transition planning requires each pass exactly once, but execution order contains pass "
                            "index {} ('{}') more than once",
                            passIndex,
                            renderPasses[passIndex].getName()
                        ),
                        .passIndex = passIndex,
                        .passName = renderPasses[passIndex].getName()
                    }
                };

            visitedPasses[passIndex] = true;
        }

        auto trackers = buildInitialTrackedStates();
        if (!trackers)
            return std::unexpected{std::move(trackers).error()};

        if (trackers->size() != nodes.size() || plan.resources.size() != nodes.size())
            return std::unexpected{
                FrameGraphError{
                    .code = FrameGraphErrorCode::InvalidGraphInvariant,
                    .message = std::format(
                        "Transition planning requires one tracker and one compiled resource table per logical "
                        "resource, but found {} trackers, {} compiled resource tables, and {} logical resources",
                        trackers->size(),
                        plan.resources.size(),
                        nodes.size()
                    )
                }
            };

        TransitionPlan transitionPlan{
            .beforePass = std::vector<std::vector<Transition>>(renderPasses.size()),
            .finalTransitions = {}
        };

        for (const uint32_t passIndex : plan.executionOrder) {
            const auto& pass = renderPasses[passIndex];
            const auto& compiledPass = plan.passes[passIndex];
            auto& transitions = transitionPlan.beforePass[passIndex];

            for (std::size_t accessIndex = 0; accessIndex < compiledPass.resourceAccesses.size(); accessIndex++) {
                const auto& access = compiledPass.resourceAccesses[accessIndex];

                if (!access.handle.isValid())
                    return std::unexpected{
                        FrameGraphError{
                            .code = FrameGraphErrorCode::InvalidGraphInvariant,
                            .message = std::format(
                                "Compiled resource access {} in pass '{}' contains an invalid handle "
                                "(index={}, version={})",
                                accessIndex,
                                pass.getName(),
                                access.handle.index,
                                access.handle.version
                            ),
                            .passIndex = passIndex,
                            .resourceVersion = access.handle.version,
                            .passName = pass.getName()
                        }
                    };

                if (access.handle.index >= trackers->size())
                    return std::unexpected{
                        FrameGraphError{
                            .code = FrameGraphErrorCode::InvalidGraphInvariant,
                            .message = std::format(
                                "Compiled resource access {} in pass '{}' references resource index {} at version "
                                "{}, but only {} tracked resources exist",
                                accessIndex,
                                pass.getName(),
                                access.handle.index,
                                access.handle.version,
                                trackers->size()
                            ),
                            .passIndex = passIndex,
                            .resourceIndex = access.handle.index,
                            .resourceVersion = access.handle.version,
                            .passName = pass.getName()
                        }
                    };

                auto result = planResourceAccess(
                    (*trackers)[access.handle.index],
                    access,
                    transitions
                );
                if (!result) {
                    auto error = std::move(result).error();
                    error.passIndex = passIndex;
                    error.passName = pass.getName();
                    appendStateDetails(
                        error,
                        (*trackers)[access.handle.index].state,
                        access.state
                    );
                    error.details.push_back(
                        std::format(
                            "The failure occurred while planning compiled resource access {} in declaration order",
                            accessIndex
                        )
                    );

                    return std::unexpected{std::move(error)};
                }

                (*trackers)[access.handle.index].used = true;
            }
        }

        for (uint32_t resourceIndex = 0; resourceIndex < nodes.size(); resourceIndex++) {
            const auto& node = nodes[resourceIndex];
            auto& tracker = (*trackers)[resourceIndex];

            if (node.lifetime != ResourceLifetime::Imported ||
                !tracker.used)
                continue;

            if (!node.finalState.has_value())
                return std::unexpected{
                    resourceError(
                        FrameGraphErrorCode::InvalidGraphInvariant,
                        "Cannot plan the final transition for imported resource '" + node.name +
                        "': the validated resource has no declared final state",
                        resourceIndex,
                        tracker.lastHandle.version
                    )
                };

            if (!tracker.state.has_value())
                return std::unexpected{
                    resourceError(
                        FrameGraphErrorCode::InvalidGraphInvariant,
                        "Cannot plan the final transition for imported resource '" + node.name +
                        "': the used resource has no tracked state after pass planning",
                        resourceIndex,
                        tracker.lastHandle.version
                    )
                };

            if (!tracker.lastHandle.isValid())
                return std::unexpected{
                    resourceError(
                        FrameGraphErrorCode::InvalidGraphInvariant,
                        std::format(
                            "Cannot plan the final transition for imported resource '{}': its last tracked handle "
                            "is invalid (index={}, version={})",
                            node.name,
                            tracker.lastHandle.index,
                            tracker.lastHandle.version
                        ),
                        resourceIndex,
                        tracker.lastHandle.version
                    )
                };

            if (tracker.lastHandle.index != resourceIndex)
                return std::unexpected{
                    resourceError(
                        FrameGraphErrorCode::InvalidGraphInvariant,
                        std::format(
                            "Cannot plan the final transition for imported resource '{}': its last tracked handle "
                            "references resource index {} instead of {}",
                            node.name,
                            tracker.lastHandle.index,
                            resourceIndex
                        ),
                        resourceIndex,
                        tracker.lastHandle.version
                    )
                };

            const PassDependencies::CompiledResourceAccess finalAccess{
                .handle = tracker.lastHandle,
                .state = *node.finalState
            };

            auto result = planResourceAccess(
                tracker,
                finalAccess,
                transitionPlan.finalTransitions
            );
            if (!result) {
                auto error = std::move(result).error();
                appendStateDetails(error, tracker.state, finalAccess.state);
                error.details.emplace_back(
                    "The failure occurred while planning the imported resource's final boundary transition");

                return std::unexpected{std::move(error)};
            }
        }

        plan.transitions = std::move(transitionPlan);

        return {};
    }

    auto FrameGraph::Builder::resolveTransitions(
        const FrameGraphResourceView& resources,
        const std::span<const Transition> transitions,
        const std::optional<uint32_t> passIndex
    ) const -> std::expected<BarrierBatcher, FrameGraphError> {
        if (passIndex.has_value() && *passIndex >= renderPasses.size())
            return std::unexpected{
                FrameGraphError{
                    .code = FrameGraphErrorCode::InvalidGraphInvariant,
                    .message = std::format(
                        "Cannot resolve pre-pass transitions for pass index {}: the builder contains {} render passes",
                        *passIndex,
                        renderPasses.size()
                    ),
                    .passIndex = *passIndex
                }
            };

        BarrierBatcher batcher;

        for (std::size_t transitionIndex = 0; transitionIndex < transitions.size(); transitionIndex++) {
            const auto& transition = transitions[transitionIndex];

            if (transition.valueless_by_exception()) {
                FrameGraphError error{
                    .code = FrameGraphErrorCode::InvalidGraphInvariant,
                    .message = std::format(
                        "Cannot resolve planned transition {} because its variant is valueless",
                        transitionIndex
                    )
                };

                if (passIndex.has_value()) {
                    error.passIndex = *passIndex;
                    error.passName = renderPasses[*passIndex].getName();
                }

                return std::unexpected{std::move(error)};
            }

            auto result = std::visit(
                [&resources, &batcher]<typename T>(const T& typedTransition)
                -> std::expected<void, FrameGraphError> {
                    using PlannedTransition = std::remove_cvref_t<T>;

                    if constexpr (std::is_same_v<PlannedTransition, ImageTransition>) {
                        auto image = resources.get(typedTransition.handle);
                        if (!image)
                            return std::unexpected{std::move(image).error()};

                        batcher.addImageBarrier(
                            typedTransition.source,
                            typedTransition.destination,
                            ImageBarrier{
                                .image = *image,
                                .sourceAccess = typedTransition.source.access,
                                .destinationAccess = typedTransition.destination.access,
                                .oldLayout = typedTransition.source.layout,
                                .newLayout = typedTransition.destination.layout,
                                .subresources = typedTransition.subresources
                            }
                        );
                    } else {
                        static_assert(std::is_same_v<PlannedTransition, BufferTransition>);

                        auto buffer = resources.get(typedTransition.handle);
                        if (!buffer)
                            return std::unexpected{std::move(buffer).error()};

                        batcher.addBufferBarrier(
                            typedTransition.source,
                            typedTransition.destination,
                            BufferBarrier{
                                .buffer = *buffer,
                                .sourceAccess = typedTransition.source.access,
                                .destinationAccess = typedTransition.destination.access,
                                .offset = typedTransition.offset,
                                .size = typedTransition.size
                            }
                        );
                    }

                    return {};
                },
                transition
            );

            if (!result) {
                auto error = std::move(result).error();
                const auto handle = std::visit(
                    [](const auto& typedTransition) {
                        return typedTransition.handle.id;
                    },
                    transition
                );
                const auto transitionType = std::holds_alternative<ImageTransition>(transition)
                                                ? "image"
                                                : "buffer";

                error.message = std::format(
                    "Failed to resolve planned {} transition {} for resource index {} at version {}: {}",
                    transitionType,
                    transitionIndex,
                    handle.index,
                    handle.version,
                    error.message
                );
                error.resourceVersion = handle.version;

                if (handle.isValid())
                    error.resourceIndex = handle.index;

                if (handle.isValid() && handle.index < nodes.size())
                    error.resourceName = nodes[handle.index].name;

                if (passIndex.has_value()) {
                    error.passIndex = *passIndex;
                    error.passName = renderPasses[*passIndex].getName();
                }

                error.details.emplace_back(
                    std::format(
                        "The lookup failure occurred while resolving declaration-order {} transition {}",
                        transitionType,
                        transitionIndex
                    )
                );
                std::visit(
                    [&error](const auto& typedTransition) {
                        appendStateDetails(
                            error,
                            ResourceState{typedTransition.source},
                            ResourceState{typedTransition.destination}
                        );
                    },
                    transition
                );

                return std::unexpected{std::move(error)};
            }
        }

        return batcher;
    }

    auto FrameGraph::Builder::resolveTransitionPlan(
        const FrameGraphResourceView& resources,
        const TransitionPlan& transitionPlan
    ) const -> std::expected<BarrierPlan, FrameGraphError> {
        if (transitionPlan.beforePass.size() != renderPasses.size())
            return std::unexpected{
                FrameGraphError{
                    .code = FrameGraphErrorCode::InvalidGraphInvariant,
                    .message = std::format(
                        "Cannot resolve the transition plan: it contains pre-pass transitions for {} passes, "
                        "but the builder contains {} render passes",
                        transitionPlan.beforePass.size(),
                        renderPasses.size()
                    ),
                    .details = {
                        "The logical transition plan must contain exactly one pre-pass transition list for every render pass"
                    }
                }
            };

        BarrierPlan resolvedPlan{
            .beforePass = std::vector<std::vector<BarrierBatch>>(renderPasses.size()),
            .finalBatches = {}
        };

        for (uint32_t passIndex = 0; passIndex < transitionPlan.beforePass.size(); passIndex++) {
            auto batcher = resolveTransitions(
                resources,
                transitionPlan.beforePass[passIndex],
                passIndex
            );
            if (!batcher)
                return std::unexpected{std::move(batcher).error()};

            resolvedPlan.beforePass[passIndex] = std::move(*batcher).takeBatches();
        }

        auto finalBatcher = resolveTransitions(
            resources,
            transitionPlan.finalTransitions,
            std::nullopt
        );
        if (!finalBatcher)
            return std::unexpected{std::move(finalBatcher).error()};

        resolvedPlan.finalBatches = std::move(*finalBatcher).takeBatches();
        return resolvedPlan;
    }

    auto FrameGraph::Builder::validateExecutionPlan(
        const DependencyPlan& dependencyPlan,
        const BarrierPlan& barrierPlan,
        const FrameGraphResourceStorage& storage
    ) const -> std::expected<void, FrameGraphError> {
        const auto passCount = renderPasses.size();
        if (dependencyPlan.passes.size() != passCount ||
            dependencyPlan.executionOrder.size() != passCount ||
            dependencyPlan.transitions.beforePass.size() != passCount ||
            barrierPlan.beforePass.size() != passCount)
            return std::unexpected{
                FrameGraphError{
                    .code = FrameGraphErrorCode::InvalidGraphInvariant,
                    .message = std::format(
                        "Cannot build an executable frame graph: expected one compiled pass, execution-order "
                        "entry, logical transition list, and resolved barrier list for each of {} declared "
                        "passes, but found {}, {}, {}, and {}, respectively",
                        passCount,
                        dependencyPlan.passes.size(),
                        dependencyPlan.executionOrder.size(),
                        dependencyPlan.transitions.beforePass.size(),
                        barrierPlan.beforePass.size()
                    )
                }
            };

        if (dependencyPlan.resources.size() != nodes.size() || storage.size() != nodes.size())
            return std::unexpected{
                FrameGraphError{
                    .code = FrameGraphErrorCode::InvalidGraphInvariant,
                    .message = std::format(
                        "Cannot build an executable frame graph: expected one compiled resource table and one "
                        "physical slot for each of {} logical resources, but found {} and {}, respectively",
                        nodes.size(),
                        dependencyPlan.resources.size(),
                        storage.size()
                    )
                }
            };

        if (!storage.isFullyResolved())
            return std::unexpected{
                FrameGraphError{
                    .code = FrameGraphErrorCode::UnresolvedResource,
                    .message =
                    "Cannot build an executable frame graph because one or more physical resource slots are unresolved"
                }
            };

        std::vector scheduledPasses(passCount, false);
        for (std::size_t orderIndex = 0; orderIndex < dependencyPlan.executionOrder.size(); orderIndex++) {
            const auto passIndex = dependencyPlan.executionOrder[orderIndex];

            if (passIndex >= passCount)
                return std::unexpected{
                    FrameGraphError{
                        .code = FrameGraphErrorCode::InvalidGraphInvariant,
                        .message = std::format(
                            "Cannot build an executable frame graph: execution-order entry {} references pass "
                            "index {}, but the graph contains {} passes",
                            orderIndex,
                            passIndex,
                            passCount
                        ),
                        .passIndex = passIndex
                    }
                };

            if (scheduledPasses[passIndex])
                return std::unexpected{
                    FrameGraphError{
                        .code = FrameGraphErrorCode::InvalidGraphInvariant,
                        .message = std::format(
                            "Cannot build an executable frame graph: pass index {} ('{}') appears more than once "
                            "in the execution order",
                            passIndex,
                            renderPasses[passIndex].getName()
                        ),
                        .passIndex = passIndex,
                        .passName = renderPasses[passIndex].getName()
                    }
                };

            scheduledPasses[passIndex] = true;
        }

        return {};
    }

    auto FrameGraph::Builder::validateDeviceLimits(
        const uint32_t maxColorAttachments
    ) const -> std::expected<void, FrameGraphError> {
        for (uint32_t passIndex = 0; passIndex < renderPasses.size(); passIndex++) {
            const auto attachmentCount = renderPasses[passIndex].getColorAttachments().size();
            if (attachmentCount > maxColorAttachments)
                return std::unexpected{
                    passError(
                        FrameGraphErrorCode::InvalidAttachment,
                        std::format(
                            "Pass '{}' declares {} color attachments, but the rendering device supports at most {}",
                            renderPasses[passIndex].getName(),
                            attachmentCount,
                            maxColorAttachments
                        ),
                        passIndex
                    )
                };
        }

        return {};
    }

    auto FrameGraph::Builder::buildExecutionPlan(
        const DependencyPlan& dependencyPlan,
        const std::vector<ResourceNode>& nodes,
        const std::vector<RenderPass>& renderPasses,
        const FrameGraphResourceStorage& storage
    ) -> std::expected<ExecutionPlan, FrameGraphError> {
        ExecutionPlan executionPlan{};
        executionPlan.passes.reserve(dependencyPlan.passes.size());

        auto resourceView = FrameGraphResourceView{storage.getResources()};
        for (const auto& passIndex : dependencyPlan.executionOrder) {
            const auto& pass = renderPasses[passIndex];
            const auto& compiledPass = dependencyPlan.passes[passIndex];

            const auto attachmentLookupError = [&nodes, &pass, passIndex](
                FrameGraphError error,
                const RenderAttachment& attachment,
                const std::string& attachmentName
            ) {
                const auto lookupMessage = std::move(error.message);
                error.message = std::format(
                    "Cannot resolve {} for pass '{}' (index {}), resource index {}, version {}: {}",
                    attachmentName,
                    pass.getName(),
                    passIndex,
                    attachment.handle.id.index,
                    attachment.handle.id.version,
                    lookupMessage
                );
                error.passIndex = passIndex;
                error.passName = pass.getName();
                error.resourceIndex = attachment.handle.id.index;
                error.resourceVersion = attachment.handle.id.version;
                if (attachment.handle.id.index < nodes.size())
                    error.resourceName = nodes[attachment.handle.id.index].name;

                return error;
            };

            const auto getAttachmentLayout = [&](
                const RenderAttachment& attachment,
                const std::string& attachmentName,
                const ImageUsageBits expectedUsage,
                const FrameGraphAttachmentRole expectedRole
            ) -> std::expected<ImageLayout, FrameGraphError> {
                const auto expectedAccess = attachment.loadAction == LoadAction::Load
                                                ? ResourceAccess::ReadWrite
                                                : ResourceAccess::Write;
                const auto authorization = authorizeFrameGraphResourceAccess(
                    compiledPass.permissions,
                    FrameGraphResourceAccessRequest{
                        .handle = attachment.handle.id,
                        .type = ResourceType::Image,
                        .access = expectedAccess,
                        .usage = FrameGraphResourceUsageKind{expectedUsage},
                        .attachmentRole = expectedRole
                    },
                    FrameGraphResourceAccessContext{
                        .passIndex = passIndex,
                        .passName = pass.getName(),
                        .resourceName = attachment.handle.id.index < nodes.size()
                                            ? std::string_view{nodes[attachment.handle.id.index].name}
                                            : std::string_view{},
                        .sideEffecting = pass.isSideEffecting(),
                        .usesExternallySynchronizedResources = pass.usesExternalResources()
                    }
                );
                if (!authorization) {
                    auto accessError = std::move(authorization).error();
                    FrameGraphError error{
                        .code = FrameGraphErrorCode::InvalidGraphInvariant,
                        .message = std::format(
                            "Cannot build {} for pass '{}' (index {}): compiled attachment authorization failed: {}",
                            attachmentName,
                            pass.getName(),
                            passIndex,
                            accessError.message
                        ),
                        .passIndex = passIndex,
                        .resourceIndex = attachment.handle.id.index,
                        .resourceVersion = attachment.handle.id.version,
                        .passName = pass.getName(),
                        .details = std::move(accessError.details)
                    };
                    if (attachment.handle.id.index < nodes.size())
                        error.resourceName = nodes[attachment.handle.id.index].name;

                    return std::unexpected{std::move(error)};
                }

                const auto compiledAccess = std::ranges::find_if(
                    compiledPass.resourceAccesses,
                    [&attachment](const PassDependencies::CompiledResourceAccess& access) {
                        // Exact output-version authority was established above.
                        // Each pass may declare a logical resource only once, so
                        // its one compiled state record is uniquely identified by index.
                        return access.handle.index == attachment.handle.id.index;
                    }
                );

                if (compiledAccess == compiledPass.resourceAccesses.end())
                    return std::unexpected{
                        FrameGraphError{
                            .code = FrameGraphErrorCode::InvalidGraphInvariant,
                            .message = std::format(
                                "Cannot build {} for pass '{}' (index {}): resource '{}' (index {}, attachment "
                                "version {}) has no compiled access in the pass dependency plan",
                                attachmentName,
                                pass.getName(),
                                passIndex,
                                nodes[attachment.handle.id.index].name,
                                attachment.handle.id.index,
                                attachment.handle.id.version
                            ),
                            .passIndex = passIndex,
                            .resourceIndex = attachment.handle.id.index,
                            .resourceVersion = attachment.handle.id.version,
                            .passName = pass.getName(),
                            .resourceName = nodes[attachment.handle.id.index].name
                        }
                    };

                const auto* imageState = std::get_if<ImageState>(&compiledAccess->state);
                if (!imageState)
                    return std::unexpected{
                        FrameGraphError{
                            .code = FrameGraphErrorCode::InvalidGraphInvariant,
                            .message = std::format(
                                "Cannot build {} for pass '{}' (index {}): the compiled access for resource '{}' "
                                "(index {}, attachment version {}, compiled version {}) contains a {} state; "
                                "render attachments require an image state",
                                attachmentName,
                                pass.getName(),
                                passIndex,
                                nodes[attachment.handle.id.index].name,
                                attachment.handle.id.index,
                                attachment.handle.id.version,
                                compiledAccess->handle.version,
                                resourceStateKind(compiledAccess->state)
                            ),
                            .passIndex = passIndex,
                            .resourceIndex = attachment.handle.id.index,
                            .resourceVersion = attachment.handle.id.version,
                            .passName = pass.getName(),
                            .resourceName = nodes[attachment.handle.id.index].name
                        }
                    };

                return imageState->layout;
            };

            PassExecutionRecord record{
                .passIndex = passIndex,
                .renderingInfo = std::nullopt,
                .debugLabelColor = pass.getDebugLabelColor(),
                .permissions = compiledPass.permissions,
                .sideEffecting = pass.isSideEffecting(),
                .usesExternallySynchronizedResources = pass.usesExternalResources()
            };

            std::vector<AttachmentInfo> colorAttachments{};
            colorAttachments.reserve(pass.getColorAttachments().size());
            for (std::size_t attachmentIndex = 0;
                 attachmentIndex < pass.getColorAttachments().size();
                 attachmentIndex++) {
                const auto& colorAttachment = pass.getColorAttachments()[attachmentIndex];
                auto image = resourceView.get(colorAttachment.handle);

                if (!image)
                    return std::unexpected{
                        attachmentLookupError(
                            std::move(image).error(),
                            colorAttachment,
                            std::format("color attachment {}", attachmentIndex)
                        )
                    };

                auto layout = getAttachmentLayout(
                    colorAttachment,
                    std::format("color attachment {}", attachmentIndex),
                    ImageUsageBits::ColorAttachment,
                    FrameGraphAttachmentRole::Color
                );
                if (!layout)
                    return std::unexpected{std::move(layout).error()};

                colorAttachments.push_back(AttachmentInfo{
                    .image = *image,
                    .layout = *layout,
                    .loadAction = colorAttachment.loadAction,
                    .storeAction = colorAttachment.storeAction,
                    .resolveImage = nullptr,
                    .clearValue = colorAttachment.clearValue
                });
            }

            std::optional<AttachmentInfo> depthStencilAttachment = std::nullopt;
            if (const auto& depthStencil = pass.getDepthStencilAttachment();
                depthStencil.has_value()) {
                auto image = resourceView.get(depthStencil->handle);

                if (!image)
                    return std::unexpected{
                        attachmentLookupError(
                            std::move(image).error(),
                            *depthStencil,
                            "depth-stencil attachment"
                        )
                    };

                auto layout = getAttachmentLayout(
                    *depthStencil,
                    "depth-stencil attachment",
                    ImageUsageBits::DepthStencilAttachment,
                    FrameGraphAttachmentRole::DepthStencil
                );
                if (!layout)
                    return std::unexpected{std::move(layout).error()};

                depthStencilAttachment = AttachmentInfo{
                    .image = *image,
                    .layout = *layout,
                    .loadAction = depthStencil->loadAction,
                    .storeAction = depthStencil->storeAction,
                    .resolveImage = nullptr,
                    .clearValue = depthStencil->clearValue
                };
            }

            if (!colorAttachments.empty() ||
                depthStencilAttachment.has_value()) {
                const auto& referenceAttachment = !colorAttachments.empty()
                                                      ? pass.getColorAttachments().front()
                                                      : *pass.getDepthStencilAttachment();
                const auto& description = std::get<ImageResourceDescription>(
                    nodes[referenceAttachment.handle.id.index].description);

                record.renderingInfo = {
                    .extent = {
                        description.format.width,
                        description.format.height,
                    },
                    .layerCount = description.format.layerCount,
                    .colorAttachments = std::move(colorAttachments),
                    .depthStencilAttachment = depthStencilAttachment
                };
            }

            executionPlan.passes.push_back(std::move(record));
        }

        return executionPlan;
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
            throw std::invalid_argument{
                "Cannot declare a frame graph resource with an empty name; resource names must be non-empty and unique"
            };

        if (resourceNames.contains(name))
            throw std::invalid_argument{
                "Cannot declare resource '" + name +
                "': another frame graph resource already uses that name; resource names must be unique"
            };

        if (nodes.size() >= ResourceId::Invalid)
            throw std::overflow_error{
                "Cannot declare resource '" + name + "': the frame graph already contains " +
                std::to_string(nodes.size()) + " resources, which is the maximum supported by ResourceId"
            };

        const bool imported = lifetime == ResourceLifetime::Imported;
        const bool hasImportedResource = !std::holds_alternative<std::monostate>(importedResource);
        if (imported != hasImportedResource ||
            imported != initialState.has_value() ||
            imported != finalState.has_value()) {
            const auto lifetimeName = lifetime == ResourceLifetime::Imported
                                          ? "imported"
                                          : lifetime == ResourceLifetime::Persistent
                                          ? "persistent"
                                          : "transient";

            throw std::logic_error{
                "Resource '" + name + "' is declared with " + lifetimeName +
                " lifetime but has inconsistent ownership metadata: imported object is " +
                (hasImportedResource ? "present" : "missing") + ", initial state is " +
                (initialState.has_value() ? "present" : "missing") + ", and final state is " +
                (finalState.has_value() ? "present" : "missing") +
                (imported
                     ? "; imported resources require all three"
                     : "; transient and persistent resources require all three to be absent")
            };
        }

        if (imported) {
            const bool importedTypeMatches = type == ResourceType::Image
                                                 ? std::holds_alternative<Image*>(importedResource) &&
                                                 std::holds_alternative<ImageState>(*initialState) &&
                                                 std::holds_alternative<ImageState>(*finalState)
                                                 : std::holds_alternative<Buffer*>(importedResource) &&
                                                 std::holds_alternative<BufferState>(*initialState) &&
                                                 std::holds_alternative<BufferState>(*finalState);
            if (!importedTypeMatches) {
                const auto importedObjectType = std::visit(
                    []<typename T>(const T& value) -> const char* {
                        using Imported = std::remove_cvref_t<T>;

                        if constexpr (std::is_same_v<Imported, std::monostate>)
                            return "none";
                        else if constexpr (std::is_same_v<Imported, Image*>)
                            return value == nullptr ? "null Image*" : "Image*";
                        else
                            return value == nullptr ? "null Buffer*" : "Buffer*";
                    }, importedResource);

                const auto stateType = [](const ResourceState& state) {
                    return std::holds_alternative<ImageState>(state) ? "ImageState" : "BufferState";
                };

                const auto expectedObjectType = type == ResourceType::Image ? "Image*" : "Buffer*";
                const auto expectedStateType = type == ResourceType::Image ? "ImageState" : "BufferState";

                throw std::logic_error{
                    "Imported resource '" + name + "' is declared as an " +
                    (type == ResourceType::Image ? "image" : "buffer") +
                    "; expected object=" + expectedObjectType + ", initial state=" +
                    expectedStateType + ", and final state=" + expectedStateType +
                    ", but received object=" + importedObjectType + ", initial state=" +
                    stateType(*initialState) + ", and final state=" + stateType(*finalState)
                };
            }
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

    auto FrameGraph::Builder::compile() const -> std::expected<DependencyPlan, FrameGraphError> {
        const auto nodeCount = nodes.size();

        DependencyPlan plan{};
        plan.resources.reserve(nodeCount);

        if (auto result = validateAndInitializeResources(plan); !result)
            return std::unexpected{std::move(result).error()};

        const auto passCount = renderPasses.size();
        plan.passes.resize(passCount);
        plan.executionOrder.reserve(passCount);

        std::vector<uint32_t> declaredVersions(nodeCount, 0);
        if (auto result = recordPassUsages(plan, declaredVersions); !result)
            return std::unexpected{std::move(result).error()};

        if (auto result = validatePassStages(); !result)
            return std::unexpected{std::move(result).error()};

        if (auto result = validateAttachments(plan); !result)
            return std::unexpected{std::move(result).error()};

        if (auto result = validateVersionTable(plan, declaredVersions); !result)
            return std::unexpected{std::move(result).error()};

        if (auto result = buildDependencyEdges(plan); !result)
            return std::unexpected{std::move(result).error()};

        if (auto result = buildExecutionOrder(plan); !result)
            return std::unexpected{std::move(result).error()};

        if (auto result = buildTransitionPlan(plan); !result)
            return std::unexpected{std::move(result).error()};

        return plan;
    }

    auto FrameGraph::Builder::allocateResources(
        RenderingDevice& device
    ) const -> std::expected<FrameGraphResourceStorage, FrameGraphError> {
        FrameGraphResourceStorage localStorage{device, std::span(nodes)};

        for (size_t resourceIndex = 0; resourceIndex < nodes.size(); resourceIndex++) {
            const auto& resource = nodes[resourceIndex];

            switch (resource.lifetime) {
                case ResourceLifetime::Transient:
                case ResourceLifetime::Persistent: {
                    switch (resource.type) {
                        case ResourceType::Image: {
                            const auto description = std::get<ImageResourceDescription>(resource.description);

                            const auto image = device.createImage(
                                description.format,
                                description.view
                            );
                            if (!image)
                                return std::unexpected{
                                    allocationError(
                                        FrameGraphErrorCode::ResourceAllocationFailed,
                                        std::format(
                                            "Failed to create backing image for resource '{}' at index {}: {}",
                                            resource.name,
                                            resourceIndex,
                                            image.error().message
                                        ),
                                        resourceIndex,
                                        image.error()
                                    )
                                };

                            if (!*image)
                                return std::unexpected{
                                    allocationError(
                                        FrameGraphErrorCode::ResourceAllocationFailed,
                                        std::format(
                                            "The driver returned a null backing image for resource '{}' at index {}",
                                            resource.name,
                                            resourceIndex
                                        ),
                                        resourceIndex
                                    )
                                };

                            try {
                                localStorage.setOwned(resourceIndex, *image);
                            } catch (...) {
                                device.getRenderingDeviceDriver()->destroyImage(*image);
                                throw;
                            }
                            break;
                        }

                        case ResourceType::Buffer: {
                            const auto description = std::get<BufferFormat>(resource.description);

                            const auto buffer = device.createBuffer(
                                description.size,
                                description.usage,
                                MemoryAllocationType::Gpu
                            );
                            if (!buffer)
                                return std::unexpected{
                                    allocationError(
                                        FrameGraphErrorCode::ResourceAllocationFailed,
                                        std::format(
                                            "Failed to create backing buffer for resource '{}' at index {}: {}",
                                            resource.name,
                                            resourceIndex,
                                            buffer.error().message
                                        ),
                                        resourceIndex,
                                        buffer.error()
                                    )
                                };

                            if (!*buffer)
                                return std::unexpected{
                                    allocationError(
                                        FrameGraphErrorCode::ResourceAllocationFailed,
                                        std::format(
                                            "The driver returned a null backing buffer for resource '{}' at index {}",
                                            resource.name,
                                            resourceIndex
                                        ),
                                        resourceIndex
                                    )
                                };

                            try {
                                localStorage.setOwned(resourceIndex, *buffer);
                            } catch (...) {
                                device.getRenderingDeviceDriver()->destroyBuffer(*buffer);
                                throw;
                            }
                            break;
                        }
                    }

                    break;
                }

                case ResourceLifetime::Imported: {
                    switch (resource.type) {
                        case ResourceType::Image: {
                            const auto image = std::get_if<Image*>(&resource.importedResource);
                            if (!image || !*image)
                                return std::unexpected{
                                    allocationError(
                                        FrameGraphErrorCode::InvalidResourceOwnership,
                                        std::format(
                                            "Resource '{}' at index {} does not contain a non-null Image pointer",
                                            resource.name,
                                            resourceIndex
                                        ),
                                        resourceIndex
                                    )
                                };

                            localStorage.setImported(resourceIndex, *image);
                            break;
                        }

                        case ResourceType::Buffer: {
                            const auto buffer = std::get_if<Buffer*>(&resource.importedResource);
                            if (!buffer || !*buffer)
                                return std::unexpected{
                                    allocationError(
                                        FrameGraphErrorCode::InvalidResourceOwnership,
                                        std::format(
                                            "Resource '{}' at index {} does not contain a non-null Buffer pointer",
                                            resource.name,
                                            resourceIndex
                                        ),
                                        resourceIndex
                                    )
                                };

                            localStorage.setImported(resourceIndex, *buffer);
                            break;
                        }
                    }

                    break;
                }
            }
        }

        return localStorage;
    }

    ImageHandle FrameGraph::Builder::createImage(
        std::string name,
        ImageResourceDescription description,
        const ResourceLifetime lifetime
    ) {
        if (lifetime == ResourceLifetime::Imported)
            throw std::invalid_argument{
                "Cannot create image resource '" + name +
                "' with imported lifetime; use importImage to provide the external image and its initial and final states"
            };

        if (description.format.width == 0 || description.format.height == 0 || description.format.depth == 0 ||
            description.format.layerCount == 0 || description.format.mipmapCount == 0)
            throw std::invalid_argument{
                "Image resource '" + name + "' has invalid dimensions or subresource counts: width=" +
                std::to_string(description.format.width) + ", height=" +
                std::to_string(description.format.height) + ", depth=" +
                std::to_string(description.format.depth) + ", layers=" +
                std::to_string(description.format.layerCount) + ", mip levels=" +
                std::to_string(description.format.mipmapCount) + "; every value must be greater than zero"
            };

        if (description.format.usage.empty())
            throw std::invalid_argument{
                "Image resource '" + name + "' has an empty usage mask; declare at least one permitted image usage"
            };

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
            throw std::invalid_argument{
                "Cannot create buffer resource '" + name +
                "' with imported lifetime; use importBuffer to provide the external buffer and its initial and final states"
            };

        if (description.size == 0)
            throw std::invalid_argument{
                "Buffer resource '" + name + "' has invalid element dimensions: size=" +
                std::to_string(description.size) + "; size must be greater than zero"
            };

        if (description.usage.empty())
            throw std::invalid_argument{
                "Buffer resource '" + name + "' has an empty usage mask; declare at least one permitted buffer usage"
            };

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
                    .size = buffer.getSize(),
                    .usage = buffer.getUsage()
                },
                &buffer,
                initialState,
                finalState
            )
        };
    }

    auto FrameGraph::Builder::build(
        RenderingDevice& device
    ) && -> std::expected<FrameGraph, FrameGraphError> {
        auto plan = compile();

        if (!plan)
            return std::unexpected{std::move(plan).error()};

        if (auto result = validateDeviceLimits(
                device.getRenderingDeviceDriver()->getMaxColorAttachments()
            ); !result)
            return std::unexpected{std::move(result).error()};

        auto localStorage = allocateResources(device);
        if (!localStorage)
            return std::unexpected{std::move(localStorage).error()};

        auto resolvedPlan = resolveTransitionPlan(
            localStorage->getResources(),
            plan->transitions
        );
        if (!resolvedPlan)
            return std::unexpected{std::move(resolvedPlan).error()};

        if (auto result = validateExecutionPlan(*plan, *resolvedPlan, *localStorage); !result)
            return std::unexpected{std::move(result).error()};

        auto execPlan = buildExecutionPlan(
            *plan,
            nodes,
            renderPasses,
            *localStorage
        );
        if (!execPlan)
            return std::unexpected{std::move(execPlan).error()};

        return FrameGraph{
            device,
            std::move(nodes),
            std::move(*plan),
            std::move(*resolvedPlan),
            std::move(*localStorage),
            std::move(renderPasses),
            std::move(*execPlan)
        };
    }
}
