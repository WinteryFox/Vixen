#pragma once

#include <vk_mem_alloc.h>
#include "Instance.h"
#include "VkCommandPool.h"

namespace Vixen::Vk {
    class Device : public std::enable_shared_from_this<Device> {
        GraphicsCard gpu;

        ::VkDevice device;

        VmaAllocator allocator;

        VkSurfaceKHR surface;

        QueueFamily graphicsQueueFamily;

        VkQueue graphicsQueue;

        QueueFamily presentQueueFamily;

        VkQueue presentQueue;

        QueueFamily transferQueueFamily;

        VkQueue transferQueue;

        std::shared_ptr<VkCommandPool> transferCommandPool;

    public:
        Device(
            const Instance& instance,
            const std::vector<const char*>& extensions,
            GraphicsCard gpu,
            VkSurfaceKHR surface
        );

        ~Device();

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

        [[nodiscard]] std::shared_ptr<VkCommandPool> getTransferCommandPool() const;

        [[nodiscard]] VkQueue getQueueHandle(uint32_t queueFamilyIndex, uint32_t queueIndex = 0) const;

        [[nodiscard]] VmaAllocator getAllocator() const;
    };
}
