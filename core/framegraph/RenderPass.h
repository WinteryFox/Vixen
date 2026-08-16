#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <glm/glm.hpp>

#include "ClearValue.h"
#include "LoadAction.h"
#include "Node.h"
#include "RenderPassContext.h"
#include "RenderPassType.h"
#include "StoreAction.h"

namespace Vixen {
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

        RenderPass(
            std::string name,
            RenderPassType type,
            std::vector<ResourceUsage> resourceUsages,
            std::vector<RenderAttachment> colorAttachments,
            std::optional<RenderAttachment> depthStencilAttachment,
            ExecuteCallback executeCallback
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

        class Builder {
            std::vector<ResourceNode>& resources;

            std::string name;

            RenderPassType type;

            std::vector<ResourceUsage> resourceUsages;

            std::vector<RenderAttachment> colorAttachments;

            std::optional<RenderAttachment> depthStencilAttachment;

            template <typename Handle>
            void validateCurrent(
                const Handle handle,
                const ResourceType expectedType
            ) const {
                if (!handle.isValid())
                    throw std::invalid_argument("Invalid frame graph resource handle");

                if (handle.id.index >= resources.size())
                    throw std::out_of_range("Frame graph resource handle is out of range");

                const auto& node = resources[handle.id.index];

                if (node.type != expectedType)
                    throw std::invalid_argument("Frame graph resource type mismatch");

                if (node.latestVersion != handle.id.version)
                    throw std::invalid_argument("Stale frame graph resource handle");
            }

            template <typename Handle>
            Handle advance(
                const Handle handle,
                const ResourceType expectedType
            ) {
                validateCurrent(handle, expectedType);

                auto& node = resources[handle.id.index];

                ++node.latestVersion;

                return Handle{
                    .id = {
                        .index = handle.id.index,
                        .version = node.latestVersion
                    }
                };
            }

        public:
            Builder(std::vector<ResourceNode>& resources, std::string name, const RenderPassType type)
                : resources(resources),
                  name(std::move(name)),
                  type(type) {}

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
                validateCurrent(handle, ResourceType::Texture);

                resourceUsages.emplace_back(
                    ImageResourceUsage{
                        .handle = handle,
                        .access = ResourceAccess::Read,
                        .usage = usage,
                        .stages = stages
                    }
                );

                return handle;
            }

            ImageHandle write(
                const ImageHandle handle,
                const ImageUsageBits usage,
                const PipelineStageFlags stages
            ) {
                const auto output = advance(handle, ResourceType::Texture);

                resourceUsages.emplace_back(
                    ImageResourceUsage{
                        .handle = handle,
                        .access = ResourceAccess::Write,
                        .usage = usage,
                        .stages = stages
                    }
                );

                return output;
            }

            ImageHandle readWrite(
                const ImageHandle handle,
                const ImageUsageBits usage,
                const PipelineStageFlags stages
            ) {
                const auto output = advance(handle, ResourceType::Texture);

                resourceUsages.emplace_back(
                    ImageResourceUsage{
                        .handle = handle,
                        .access = ResourceAccess::ReadWrite,
                        .usage = usage,
                        .stages = stages
                    }
                );

                return output;
            }

            BufferHandle read(
                const BufferHandle handle,
                const BufferUsageBits usage,
                const PipelineStageFlags stages
            ) {
                validateCurrent(handle, ResourceType::Buffer);

                resourceUsages.emplace_back(
                    BufferResourceUsage{
                        .handle = handle,
                        .access = ResourceAccess::Read,
                        .usage = usage,
                        .stages = stages
                    }
                );

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
                        .handle = handle,
                        .access = ResourceAccess::Write,
                        .usage = usage,
                        .stages = stages
                    }
                );

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
                        .handle = handle,
                        .access = ResourceAccess::ReadWrite,
                        .usage = usage,
                        .stages = stages
                    }
                );

                return output;
            }

            ImageHandle addColorAttachment(
                const ImageHandle handle,
                const LoadAction loadAction,
                const StoreAction storeAction,
                const ClearValue clearValue = {}
            ) {
                if (type != RenderPassType::Graphics)
                    throw std::logic_error("Compute pass cannot have color attachments");

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
                        .handle = handle,
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
                const ClearValue clearValue = {}
            ) {
                if (type != RenderPassType::Graphics)
                    throw std::logic_error("Compute pass cannot have depth stencil attachment");

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
                    .handle = handle,
                    .loadAction = loadAction,
                    .storeAction = storeAction,
                    .clearValue = clearValue
                };

                return output;
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

                return RenderPass(
                    std::move(name),
                    type,
                    std::move(resourceUsages),
                    std::move(colorAttachments),
                    std::move(depthStencilAttachment),
                    ExecuteCallback(std::move(callback))
                );
            }
        };
    };
} // namespace Vixen
