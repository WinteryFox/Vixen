#pragma once

#include <memory>
#include <vma/vk_mem_alloc.h>

#include "DeletionQueue.h"
#include "Instance.h"

namespace Vixen {
    enum class CommandPoolUsage;
    class VulkanCommandPool;

    class VulkanDevice : public std::enable_shared_from_this<VulkanDevice> {
        std::shared_ptr<Instance> instance;

        GraphicsCard gpu;

        VkDevice device;

        DeletionQueue deletionQueue;

        VmaAllocator allocator;

        VkSurfaceKHR surface;

        QueueFamily graphicsQueueFamily;

        VkQueue graphicsQueue;

        QueueFamily presentQueueFamily;

        VkQueue presentQueue;

        QueueFamily transferQueueFamily;

        VkQueue transferQueue;

        std::shared_ptr<VulkanCommandPool> transferCommandPool;

    public:
        VulkanDevice(
            const std::shared_ptr<Instance>& instance,
            const std::vector<const char*>& extensions,
            GraphicsCard gpu,
            VkSurfaceKHR surface
        );

        VulkanDevice(const VulkanDevice&) = delete;

        VulkanDevice& operator=(const VulkanDevice&) = delete;

        VulkanDevice(VulkanDevice&& other) noexcept;

        VulkanDevice& operator=(VulkanDevice&& other) noexcept;

        ~VulkanDevice();

        void waitIdle() const;

        [[nodiscard]] VkDevice getDevice() const;

        [[nodiscard]] const GraphicsCard& getGpu() const;

        [[nodiscard]] VkSurfaceKHR getSurface() const;

        [[nodiscard]] const QueueFamily& getGraphicsQueueFamily() const;

        [[nodiscard]] VkQueue getGraphicsQueue() const;

        [[nodiscard]] const QueueFamily& getTransferQueueFamily() const;

        [[nodiscard]] VkQueue getTransferQueue() const;

        [[nodiscard]] const QueueFamily& getPresentQueueFamily() const;

        [[nodiscard]] VkQueue getPresentQueue() const;

        [[nodiscard]] std::shared_ptr<VulkanCommandPool> getTransferCommandPool();

        [[nodiscard]] VkQueue getQueueHandle(uint32_t queueFamilyIndex, uint32_t queueIndex = 0) const;

        [[nodiscard]] VmaAllocator getAllocator() const;

        std::shared_ptr<VulkanCommandPool> allocateCommandPool(CommandPoolUsage usage, bool createReset);
    };
}
