#include "VkCommandBuffer.h"

namespace Vixen::Vk {
    VkCommandBuffer::VkCommandBuffer(const std::shared_ptr<VkCommandPool> &commandPool, ::VkCommandBuffer commandBuffer)
            : commandPool(commandPool),
              commandBuffer(commandBuffer) {}

    VkCommandBuffer::VkCommandBuffer(const std::shared_ptr<VkCommandPool> &commandPool, VkCommandBufferLevel level)
            : commandPool(commandPool),
              commandBuffer(VK_NULL_HANDLE) {
        VkCommandBufferAllocateInfo info{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = commandPool->getCommandPool(),
                .level = level,
                .commandBufferCount = 1
        };

        checkVulkanResult(
                vkAllocateCommandBuffers(commandPool->getDevice()->getDevice(), &info, &commandBuffer),
                "Failed to create command buffers"
        );
    }

    VkCommandBuffer::~VkCommandBuffer() {
        vkFreeCommandBuffers(commandPool->getDevice()->getDevice(), commandPool->getCommandPool(), 1, &commandBuffer);
    }
}
