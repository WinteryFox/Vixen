#pragma once

#include <memory>

#include "VulkanCommandBuffer.h"

namespace Vixen {
    enum class CommandPoolUsage;
    enum class CommandBufferLevel;

    class VulkanCommandPool final : public std::enable_shared_from_this<VulkanCommandPool> {
        std::shared_ptr<VulkanDevice> device;

        VkCommandPool commandPool;

        static std::vector<VkCommandBuffer> allocateCommandBuffers(
            VkDevice device,
            VkCommandPool commandPool,
            VkCommandBufferLevel level,
            uint32_t count
        );

    public:
        VulkanCommandPool(const std::shared_ptr<VulkanDevice>& device, uint32_t queueFamilyIndex, CommandPoolUsage usage,
                      bool createReset);

        VulkanCommandPool(const VulkanCommandPool&) = delete;

        VulkanCommandPool& operator=(const VulkanCommandPool&) = delete;

        VulkanCommandPool(VulkanCommandPool&& other) noexcept;

        VulkanCommandPool& operator=(VulkanCommandPool&& other) noexcept;

        ~VulkanCommandPool();

        std::vector<VulkanCommandBuffer> allocate(CommandBufferLevel level, uint32_t count);

        VulkanCommandBuffer allocate(CommandBufferLevel level);

        void reset() const;

        [[nodiscard]] std::shared_ptr<VulkanDevice> getDevice() const;

        [[nodiscard]] ::VkCommandPool getCommandPool() const;
    };
}
