#pragma once

#include <cstdint>
#include <vector>
#include <volk.h>

#include "core/command/CommandBuffer.h"

namespace Vixen {
    class VulkanCommandBuffer final : public CommandBuffer {
        friend class VulkanRenderingDeviceDriver;

        VkCommandBuffer commandBuffer = nullptr;

        std::vector<VkRenderingAttachmentInfo> renderingAttachmentScratch{};

    public:
        VulkanCommandBuffer(
            const QueueFamilyFlags queueCapabilities,
            const uint32_t renderingAttachmentCount,
            VkCommandBuffer commandBuffer,
            CommandPool* pool
        ) : CommandBuffer(queueCapabilities, pool),
            commandBuffer(commandBuffer) {
            renderingAttachmentScratch.reserve(renderingAttachmentCount);
        }

        VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
        VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;

        VulkanCommandBuffer(VulkanCommandBuffer&&) = delete;
        VulkanCommandBuffer& operator=(VulkanCommandBuffer&&) = delete;
    };
}
