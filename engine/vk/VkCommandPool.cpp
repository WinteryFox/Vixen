#include "VkCommandPool.h"

namespace Vixen::Vk {
    VkCommandPool::VkCommandPool(
            ::VkDevice device,
            uint32_t queueFamilyIndex,
            Usage usage,
            bool createReset
    ) : device(device),
        commandPool(VK_NULL_HANDLE) {
        VkCommandPoolCreateFlags flags;

        switch (usage) {
            case Usage::GRAPHICS:
                flags = 0;
                break;
            case Usage::TRANSIENT:
                flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
                break;
        }

        if (createReset)
            flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkCommandPoolCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = flags,
                .queueFamilyIndex = queueFamilyIndex
        };

        checkVulkanResult(
                vkCreateCommandPool(device, &info, nullptr, &commandPool),
                "Failed to create command pool"
        );
    }

    VkCommandPool::~VkCommandPool() {
        vkDestroyCommandPool(device, commandPool, nullptr);
    }

    std::vector<::VkCommandBuffer>
    VkCommandPool::allocateCommandBuffers(
            ::VkDevice device,
            ::VkCommandPool commandPool,
            VkCommandBufferLevel level,
            uint32_t count
    ) {
        std::vector<::VkCommandBuffer> commandBuffers{count};

        VkCommandBufferAllocateInfo info{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = commandPool,
                .level = level,
                .commandBufferCount = count
        };

        checkVulkanResult(
                vkAllocateCommandBuffers(device, &info, commandBuffers.data()),
                "Failed to create command buffers"
        );

        return commandBuffers;
    }

    std::vector<VkCommandBuffer>
    VkCommandPool::allocateCommandBuffers(VkCommandBuffer::Level level, uint32_t count) {
        if (count == 0)
            throw std::runtime_error("Must allocate at least one command buffer");

        auto b = allocateCommandBuffers(
                device,
                commandPool,
                static_cast<VkCommandBufferLevel>(level),
                count
        );

        std::vector<VkCommandBuffer> buffers;
        buffers.reserve(count);

        for (auto &buffer: b)
            buffers.emplace_back(device, commandPool, buffer);

        return buffers;
    }

    VkCommandBuffer VkCommandPool::allocateCommandBuffer(VkCommandBuffer::Level level) {
        return {
                device,
                commandPool,
                allocateCommandBuffers(
                        device,
                        commandPool,
                        static_cast<VkCommandBufferLevel>(level),
                        1
                )[0]
        };
    }

    ::VkCommandPool VkCommandPool::getCommandPool() const {
        return commandPool;
    }
}
