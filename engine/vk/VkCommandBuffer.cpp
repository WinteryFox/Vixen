#include "VkCommandBuffer.h"

namespace Vixen::Vk {
    VkCommandBuffer::VkCommandBuffer(const std::shared_ptr<VkCommandPool> &commandPool, ::VkCommandBuffer commandBuffer)
            : commandPool(commandPool),
              commandBuffer(commandBuffer) {}

    VkCommandBuffer::VkCommandBuffer(const std::shared_ptr<VkCommandPool> &commandPool, Level level)
            : commandPool(commandPool),
              commandBuffer(allocateCommandBuffers(commandPool, static_cast<VkCommandBufferLevel>(level), 1)[0]) {}

    VkCommandBuffer::VkCommandBuffer(VkCommandBuffer &&o) noexcept
            : commandPool(o.commandPool),
              commandBuffer(std::exchange(o.commandBuffer, nullptr)) {}

    VkCommandBuffer::~VkCommandBuffer() {
        vkFreeCommandBuffers(commandPool->getDevice()->getDevice(), commandPool->getCommandPool(), 1, &commandBuffer);
    }

    std::vector<VkCommandBuffer>
    VkCommandBuffer::allocateCommandBuffers(const std::shared_ptr<VkCommandPool> &commandPool,
                                            VkCommandBuffer::Level level, uint32_t count) {
        auto b = allocateCommandBuffers(commandPool, static_cast<VkCommandBufferLevel>(level), count);

        std::vector<VkCommandBuffer> buffers;
        buffers.reserve(count);

        for (auto &buffer: b)
            buffers.emplace_back(commandPool, buffer);

        return buffers;
    }

    std::vector<::VkCommandBuffer>
    VkCommandBuffer::allocateCommandBuffers(const std::shared_ptr<VkCommandPool> &commandPool,
                                            VkCommandBufferLevel level, uint32_t count) {
        std::vector<::VkCommandBuffer> commandBuffers{count};

        VkCommandBufferAllocateInfo info{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = commandPool->getCommandPool(),
                .level = level,
                .commandBufferCount = count
        };

        checkVulkanResult(
                vkAllocateCommandBuffers(commandPool->getDevice()->getDevice(), &info, commandBuffers.data()),
                "Failed to create command buffers"
        );

        return std::vector<::VkCommandBuffer>();
    }
}
