#include "VkCommandBuffer.h"

namespace Vixen::Vk {
    VkCommandBuffer::VkCommandBuffer(::VkDevice device, ::VkCommandPool commandPool, ::VkCommandBuffer commandBuffer)
            : device(device),
              commandPool(commandPool),
              commandBuffer(commandBuffer) {}

    VkCommandBuffer::~VkCommandBuffer() {
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void VkCommandBuffer::reset() {
        checkVulkanResult(
                vkResetCommandBuffer(commandBuffer, 0),
                "Failed to reset command buffer"
        );
    }

    ::VkCommandBuffer VkCommandBuffer::getCommandBuffer() const {
        return commandBuffer;
    }
}
