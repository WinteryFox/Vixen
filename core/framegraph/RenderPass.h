#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/rendering/ClearValue.h"
#include "core/rendering/LoadAction.h"
#include "Node.h"
#include "RenderPassType.h"
#include "core/rendering/StoreAction.h"
#include "glm/vec4.hpp"

namespace Vixen {
    struct RenderPassContext;

    struct RenderAttachment {
        ImageHandle handle;

        LoadAction loadAction;

        StoreAction storeAction;

        ClearValue clearValue;
    };

    class RenderPass {
    public:
        using ExecuteCallback = std::move_only_function<void(RenderPassContext&)>;

    private:
        std::string name;

        RenderPassType type;

        std::vector<ResourceUsage> resourceUsages;

        std::vector<RenderAttachment> colorAttachments;

        std::optional<RenderAttachment> depthStencilAttachment;

        ExecuteCallback executeCallback;

        glm::vec4 debugLabelColor;

        bool sideEffecting;

        bool usesExternallySynchronizedResources;

        RenderPass(
            std::string name,
            RenderPassType type,
            std::vector<ResourceUsage> resourceUsages,
            std::vector<RenderAttachment> colorAttachments,
            std::optional<RenderAttachment> depthStencilAttachment,
            ExecuteCallback executeCallback,
            glm::vec4 debugLabelColor,
            bool sideEffecting,
            bool usesExternallySynchronizedResources
        );

    public:
        RenderPass(const RenderPass& other) = delete;

        RenderPass(RenderPass&& other) noexcept = default;

        RenderPass& operator=(const RenderPass& other) = delete;

        RenderPass& operator=(RenderPass&& other) noexcept = default;

        ~RenderPass() = default;

        void execute(RenderPassContext& context);

        [[nodiscard]] const std::string& getName() const noexcept;

        [[nodiscard]] RenderPassType getType() const noexcept;

        [[nodiscard]] const std::vector<ResourceUsage>& getResourceUsages() const noexcept;

        [[nodiscard]] const std::vector<RenderAttachment>& getColorAttachments() const noexcept;

        [[nodiscard]] const std::optional<RenderAttachment>& getDepthStencilAttachment() const noexcept;

        [[nodiscard]] glm::vec4 getDebugLabelColor() const noexcept;

        /**
         * @brief Returns whether this pass performs an observable side effect.
         *
         * Side-effecting passes remain uncullable when pass culling is added.
         * Every frame-graph resource they touch must still be declared normally.
         */
        [[nodiscard]] bool isSideEffecting() const noexcept;

        /**
         * @brief Returns whether the callback captures externally synchronized resources.
         *
         * This is a diagnostic escape-hatch marker, not permission to bypass
         * frame-graph declarations. Captured external resources are outside the
         * graph's hazard tracking contract and must be synchronized by the caller.
         */
        [[nodiscard]] bool usesExternalResources() const noexcept;

        class Builder {
            std::vector<ResourceNode>& resources;

            std::string name;

            RenderPassType type;

            std::vector<ResourceUsage> resourceUsages;

            std::unordered_set<uint32_t> usedResourceIndices;

            std::vector<RenderAttachment> colorAttachments;

            std::optional<RenderAttachment> depthStencilAttachment;

            glm::vec4 debugLabelColor;

            bool sideEffecting = false;

            bool usesExternallySynchronizedResources = false;

            [[nodiscard]]
            static constexpr const char* resourceTypeName(const ResourceType resourceType) noexcept {
                switch (resourceType) {
                    case ResourceType::Image:
                        return "image";
                    case ResourceType::Buffer:
                        return "buffer";
                    default:
                        return "unrecognized";
                }
            }

            void validateUnused(const ResourceId id) const {
                if (usedResourceIndices.contains(id.index))
                    throw std::logic_error{
                        "Pass '" + name + "' declares resource '" + resources[id.index].name +
                        "' (index " + std::to_string(id.index) +
                        ") more than once; each pass may declare a logical resource only once"
                    };
            }

            template <typename Handle>
            void validateCurrent(
                const Handle handle,
                const ResourceType expectedType
            ) const {
                if (!handle.isValid())
                    throw std::invalid_argument{
                        "Pass '" + name + "' received an invalid " + resourceTypeName(expectedType) +
                        " resource handle (index " + std::to_string(handle.id.index) +
                        ", version " + std::to_string(handle.id.version) +
                        "); the handle must refer to a resource declared by this frame graph builder"
                    };

                if (handle.id.index >= resources.size())
                    throw std::out_of_range{
                        "Pass '" + name + "' received a " + resourceTypeName(expectedType) +
                        " resource handle with index " + std::to_string(handle.id.index) +
                        " and version " + std::to_string(handle.id.version) +
                        ", but this frame graph contains " + std::to_string(resources.size()) +
                        " resources; valid indices are in [0, " + std::to_string(resources.size()) + ")"
                    };

                const auto& node = resources[handle.id.index];

                if (node.type != expectedType)
                    throw std::invalid_argument{
                        "Pass '" + name + "' expected resource '" + node.name +
                        "' (index " + std::to_string(handle.id.index) +
                        ", version " + std::to_string(handle.id.version) + ") to be a " +
                        resourceTypeName(expectedType) + ", but it is declared as a " +
                        resourceTypeName(node.type)
                    };

                if (handle.id.version != node.latestVersion)
                    throw std::invalid_argument{
                        "Pass '" + name + "' received a stale handle for resource '" + node.name +
                        "' (index " + std::to_string(handle.id.index) + "): supplied version " +
                        std::to_string(handle.id.version) + ", but the latest version is " +
                        std::to_string(node.latestVersion)
                    };
            }

            template <typename Handle>
            Handle advance(
                const Handle handle,
                const ResourceType expectedType
            ) {
                validateCurrent(handle, expectedType);
                validateUnused(handle.id);

                auto& node = resources[handle.id.index];

                if (node.latestVersion == std::numeric_limits<uint32_t>::max())
                    throw std::overflow_error{
                        "Pass '" + name + "' cannot advance resource '" + node.name +
                        "' (index " + std::to_string(handle.id.index) +
                        ") beyond version " + std::to_string(node.latestVersion) +
                        "; the resource version limit has been reached"
                    };

                ++node.latestVersion;

                return Handle{
                    .id = {
                        .index = handle.id.index,
                        .version = node.latestVersion
                    }
                };
            }

        public:
            Builder(
                std::vector<ResourceNode>& resources,
                std::string name,
                const RenderPassType type
            ) : resources(resources),
                name(std::move(name)),
                type(type),
                debugLabelColor({}) {
                switch (type) {
                    case RenderPassType::Graphics:
                        debugLabelColor = {0.31f, 0.56f, 0.97f, 1.0f};
                        break;

                    case RenderPassType::Compute:
                        debugLabelColor = {0.61f, 0.36f, 0.9f, 1.0f};
                        break;
                }
            }

            Builder(const Builder& other) = delete;

            Builder(Builder&& other) noexcept = delete;

            Builder& operator=(const Builder& other) = delete;

            Builder& operator=(Builder&& other) noexcept = delete;

            ~Builder() = default;

            ImageHandle read(
                const ImageHandle handle,
                const ImageUsageBits usage,
                const PipelineStageFlags stages
            ) {
                validateCurrent(handle, ResourceType::Image);
                validateUnused(handle.id);

                resourceUsages.emplace_back(
                    ImageResourceUsage{
                        .input = handle,
                        .output = {},
                        .access = ResourceAccess::Read,
                        .usage = usage,
                        .stages = stages
                    }
                );
                usedResourceIndices.insert(handle.id.index);

                return handle;
            }

            ImageHandle write(
                const ImageHandle handle,
                const ImageUsageBits usage,
                const PipelineStageFlags stages
            ) {
                const auto output = advance(handle, ResourceType::Image);

                resourceUsages.emplace_back(
                    ImageResourceUsage{
                        .input = {},
                        .output = output,
                        .access = ResourceAccess::Write,
                        .usage = usage,
                        .stages = stages
                    }
                );
                usedResourceIndices.insert(handle.id.index);

                return output;
            }

            ImageHandle readWrite(
                const ImageHandle handle,
                const ImageUsageBits usage,
                const PipelineStageFlags stages
            ) {
                const auto output = advance(handle, ResourceType::Image);

                resourceUsages.emplace_back(
                    ImageResourceUsage{
                        .input = handle,
                        .output = output,
                        .access = ResourceAccess::ReadWrite,
                        .usage = usage,
                        .stages = stages
                    }
                );
                usedResourceIndices.insert(handle.id.index);

                return output;
            }

            BufferHandle read(
                const BufferHandle handle,
                const BufferUsageBits usage,
                const PipelineStageFlags stages
            ) {
                validateCurrent(handle, ResourceType::Buffer);
                validateUnused(handle.id);

                resourceUsages.emplace_back(
                    BufferResourceUsage{
                        .input = handle,
                        .output = {},
                        .access = ResourceAccess::Read,
                        .usage = usage,
                        .stages = stages
                    }
                );
                usedResourceIndices.insert(handle.id.index);

                return handle;
            }

            BufferHandle write(
                const BufferHandle handle,
                const BufferUsageBits usage,
                const PipelineStageFlags stages
            ) {
                const auto output = advance(handle, ResourceType::Buffer);

                resourceUsages.emplace_back(
                    BufferResourceUsage{
                        .input = {},
                        .output = output,
                        .access = ResourceAccess::Write,
                        .usage = usage,
                        .stages = stages
                    }
                );
                usedResourceIndices.insert(handle.id.index);

                return output;
            }

            BufferHandle readWrite(
                const BufferHandle handle,
                const BufferUsageBits usage,
                const PipelineStageFlags stages
            ) {
                const auto output = advance(handle, ResourceType::Buffer);

                resourceUsages.emplace_back(
                    BufferResourceUsage{
                        .input = handle,
                        .output = output,
                        .access = ResourceAccess::ReadWrite,
                        .usage = usage,
                        .stages = stages
                    }
                );
                usedResourceIndices.insert(handle.id.index);

                return output;
            }

            ImageHandle addColorAttachment(
                const ImageHandle handle,
                const LoadAction loadAction,
                const StoreAction storeAction,
                const ClearValue& clearValue = {}
            ) {
                if (type != RenderPassType::Graphics)
                    throw std::logic_error{
                        "Pass '" + name +
                        "' is a compute pass and cannot declare color attachments; use a graphics pass instead"
                    };

                const auto output = loadAction == LoadAction::Load
                                        ? readWrite(
                                            handle,
                                            ImageUsageBits::ColorAttachment,
                                            PipelineStageBits::ColorAttachmentOutput
                                        )
                                        : write(
                                            handle,
                                            ImageUsageBits::ColorAttachment,
                                            PipelineStageBits::ColorAttachmentOutput
                                        );

                colorAttachments.emplace_back(
                    RenderAttachment{
                        .handle = output,
                        .loadAction = loadAction,
                        .storeAction = storeAction,
                        .clearValue = clearValue
                    }
                );

                return output;
            }

            ImageHandle setDepthStencilAttachment(
                const ImageHandle handle,
                const LoadAction loadAction,
                const StoreAction storeAction,
                const ClearValue& clearValue = {}
            ) {
                if (type != RenderPassType::Graphics)
                    throw std::logic_error{
                        "Pass '" + name +
                        "' is a compute pass and cannot declare a depth-stencil attachment; use a graphics pass instead"
                    };

                if (depthStencilAttachment.has_value())
                    throw std::logic_error{
                        "Pass '" + name +
                        "' already has a depth-stencil attachment; a graphics pass may declare only one"
                    };

                constexpr PipelineStageFlags stages = PipelineStageBits::EarlyFragmentTests |
                    PipelineStageBits::LateFragmentTests;

                const auto output = loadAction == LoadAction::Load
                                        ? readWrite(
                                            handle,
                                            ImageUsageBits::DepthStencilAttachment,
                                            stages
                                        )
                                        : write(
                                            handle,
                                            ImageUsageBits::DepthStencilAttachment,
                                            stages
                                        );

                depthStencilAttachment = RenderAttachment{
                    .handle = output,
                    .loadAction = loadAction,
                    .storeAction = storeAction,
                    .clearValue = clearValue
                };

                return output;
            }

            Builder& setDebugLabelColor(const glm::vec4 color) {
                debugLabelColor = color;
                return *this;
            }

            /** Marks the pass as an uncullable producer of observable side effects. */
            Builder& setSideEffecting(const bool value = true) noexcept {
                sideEffecting = value;
                return *this;
            }

            /**
             * Marks captured non-frame-graph resources as externally synchronized.
             * This does not authorize access to undeclared frame-graph resources.
             */
            Builder& setUsesExternallySynchronizedResources(const bool value = true) noexcept {
                usesExternallySynchronizedResources = value;
                return *this;
            }

            template <typename Data, typename Execute>
            [[nodiscard]]
            RenderPass build(
                Data&& data,
                Execute&& execute
            ) && {
                using StoredData = std::decay_t<Data>;
                using StoredExecute = std::decay_t<Execute>;

                static_assert(
                    std::is_invocable_v<
                        StoredExecute&,
                        const StoredData&,
                        RenderPassContext&>,
                    "Execute callback must accept "
                    "(const PassData&, RenderPassContext&)"
                );

                auto callback = [
                        data = StoredData(std::forward<Data>(data)),
                        execute = StoredExecute(std::forward<Execute>(execute))
                    ](RenderPassContext& context) mutable {
                    std::invoke(
                        execute,
                        std::as_const(data),
                        context
                    );
                };

                return {
                    std::move(name),
                    type,
                    std::move(resourceUsages),
                    std::move(colorAttachments),
                    depthStencilAttachment,
                    ExecuteCallback(std::move(callback)),
                    debugLabelColor,
                    sideEffecting,
                    usesExternallySynchronizedResources
                };
            }
        };
    };
}
