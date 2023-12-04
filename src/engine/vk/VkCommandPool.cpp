#include "VkCommandPool.h"

namespace Vixen::Vk {
    VkCommandPool::VkCommandPool(
        ::VkDevice device,
        const uint32_t queueFamilyIndex,
        const Usage usage,
        const bool createReset
    ) : device(device),
        commandPool(VK_NULL_HANDLE) {
        VkCommandPoolCreateFlags flags = 0;

        switch (usage) {
        case Usage::GRAPHICS:
            break;
        case Usage::TRANSIENT:
            flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            break;
        }

        if (createReset)
            flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        const VkCommandPoolCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = flags,
            .queueFamilyIndex = queueFamilyIndex
        };

        checkVulkanResult(
            vkCreateCommandPool(device, &info, nullptr, &commandPool),
            "Failed to create command pool"
        );
    }

    VkCommandPool::VkCommandPool(VkCommandPool&& other) noexcept
        : device(other.device),
          commandPool(other.commandPool) {}

    VkCommandPool const& VkCommandPool::operator=(VkCommandPool&& other) noexcept {
        std::swap(device, other.device);
        std::swap(commandPool, other.commandPool);

        return *this;
    }

    VkCommandPool::~VkCommandPool() {
        vkDestroyCommandPool(device, commandPool, nullptr);
    }

    std::vector<::VkCommandBuffer> VkCommandPool::allocateCommandBuffers(
        ::VkDevice device,
        ::VkCommandPool commandPool,
        const VkCommandBufferLevel level,
        const uint32_t count
    ) {
        std::vector<::VkCommandBuffer> commandBuffers{count};

        const VkCommandBufferAllocateInfo info{
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
    VkCommandPool::allocate(VkCommandBuffer::Level level, const uint32_t count) {
        if (count == 0)
            throw std::runtime_error("Must allocate at least one command buffer");

        const auto b = allocateCommandBuffers(
            device,
            commandPool,
            static_cast<VkCommandBufferLevel>(level),
            count
        );

        std::vector<VkCommandBuffer> buffers;
        buffers.reserve(count);

        for (auto& buffer : b)
            buffers.emplace_back(device, commandPool, buffer);

        return buffers;
    }

    VkCommandBuffer VkCommandPool::allocate(VkCommandBuffer::Level level) {
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

    void VkCommandPool::reset() const {
        checkVulkanResult(
            vkResetCommandPool(device, commandPool, 0),
            "Failed to reset command pool"
        );
    }
}
