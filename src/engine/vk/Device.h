#pragma once

#include <vma/vk_mem_alloc.h>
#include "Instance.h"
#include "VkCommandPool.h"
#include "DeletionQueue.h"

namespace Vixen {
    enum class CommandPoolUsage;
}

namespace Vixen::Vk {
    class Device : public std::enable_shared_from_this<Device> {
        std::shared_ptr<Instance> instance;

        GraphicsCard gpu;

        ::VkDevice device;

        DeletionQueue deletionQueue;

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
            const std::shared_ptr<Instance>& instance,
            const std::vector<const char*>& extensions,
            GraphicsCard gpu,
            VkSurfaceKHR surface
        );

        Device(const Device&) = delete;

        Device& operator=(const Device&) = delete;

        Device(Device&& other) noexcept;

        Device& operator=(Device&& other) noexcept;

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

        [[nodiscard]] std::shared_ptr<VkCommandPool> getTransferCommandPool();

        [[nodiscard]] VkQueue getQueueHandle(uint32_t queueFamilyIndex, uint32_t queueIndex = 0) const;

        [[nodiscard]] VmaAllocator getAllocator() const;

        std::shared_ptr<VkCommandPool> allocateCommandPool(CommandPoolUsage usage, bool createReset);
    };
}
