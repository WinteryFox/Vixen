#include "VulkanCommandPool.h"

#include "device/VulkanDevice.h"
#include "core/CommandPoolUsage.h"

namespace Vixen {
    VulkanCommandPool::VulkanCommandPool(
        const std::shared_ptr<VulkanDevice>& device,
        const uint32_t queueFamilyIndex,
        const CommandPoolUsage usage,
        const bool createReset
    ) : device(device),
        commandPool(VK_NULL_HANDLE) {
        VkCommandPoolCreateFlags flags = 0;

        switch (usage) {
        case CommandPoolUsage::Graphics:
            break;
        case CommandPoolUsage::Transient:
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
            vkCreateCommandPool(device->getDevice(), &info, nullptr, &commandPool),
            "Failed to create command pool"
        );
    }

    VulkanCommandPool::VulkanCommandPool(VulkanCommandPool&& other) noexcept
        : device(std::exchange(other.device, nullptr)),
          commandPool(std::exchange(other.commandPool, nullptr)) {}

    VulkanCommandPool& VulkanCommandPool::operator=(VulkanCommandPool&& other) noexcept {
        std::swap(device, other.device);
        std::swap(commandPool, other.commandPool);

        return *this;
    }

    VulkanCommandPool::~VulkanCommandPool() {
        vkDestroyCommandPool(device->getDevice(), commandPool, nullptr);
    }

    std::vector<::VkCommandBuffer> VulkanCommandPool::allocateCommandBuffers(
        ::VkDevice device,
        ::VkCommandPool commandPool,
        const VkCommandBufferLevel level,
        const uint32_t count
    ) {
        std::vector<::VkCommandBuffer> commandBuffers{count};

        const VkCommandBufferAllocateInfo info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
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

    std::vector<VulkanCommandBuffer>
    VulkanCommandPool::allocate(CommandBufferLevel level, const uint32_t count) {
        if (count == 0)
            throw std::runtime_error("Must allocate at least one command buffer");

        const auto& b = allocateCommandBuffers(
            device->getDevice(),
            commandPool,
            static_cast<VkCommandBufferLevel>(level),
            count
        );

        std::vector<VulkanCommandBuffer> buffers;
        buffers.reserve(count);

        for (auto& buffer : b)
            buffers.emplace_back(shared_from_this(), buffer);

        return buffers;
    }

    VulkanCommandBuffer VulkanCommandPool::allocate(CommandBufferLevel level) {
        return {
            shared_from_this(),
            allocateCommandBuffers(
                device->getDevice(),
                commandPool,
                static_cast<VkCommandBufferLevel>(level),
                1
            )[0]
        };
    }

    void VulkanCommandPool::reset() const {
        checkVulkanResult(
            vkResetCommandPool(device->getDevice(), commandPool, 0),
            "Failed to reset command pool"
        );
    }

    std::shared_ptr<VulkanDevice> VulkanCommandPool::getDevice() const { return device; }

    ::VkCommandPool VulkanCommandPool::getCommandPool() const { return commandPool; }
}
