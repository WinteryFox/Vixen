#pragma once

#include <memory>
#include <set>
#include "Instance.h"
#include "Allocator.h"

namespace Vixen::Vk {
    class Device {
        GraphicsCard gpu;

        VkDevice device;

        std::shared_ptr<Allocator> allocator;

        VkSurfaceKHR surface;

        QueueFamily graphicsQueueFamily;

        VkQueue graphicsQueue;

        QueueFamily transferQueueFamily;

        VkQueue transferQueue;

        QueueFamily presentQueueFamily;

        VkQueue presentQueue;

    public:
        Device(
                const Instance &instance,
                const std::vector<const char *> &extensions,
                GraphicsCard gpu,
                VkSurfaceKHR surface
        );

        ~Device();

        [[nodiscard]] VkDevice getDevice() const;

        [[nodiscard]] const GraphicsCard &getGpu() const;

        [[nodiscard]] const std::shared_ptr<Allocator> &getAllocator() const;

        [[nodiscard]] VkSurfaceKHR getSurface() const;

        [[nodiscard]] const QueueFamily &getGraphicsQueueFamily() const;

        [[nodiscard]] VkQueue getGraphicsQueue() const;

        [[nodiscard]] const QueueFamily &getTransferQueueFamily() const;

        [[nodiscard]] VkQueue getTransferQueue() const;

        [[nodiscard]] const QueueFamily &getPresentQueueFamily() const;

        [[nodiscard]] VkQueue getPresentQueue() const;

        [[nodiscard]] VkQueue getQueueHandle(uint32_t queueFamilyIndex, uint32_t queueIndex = 0) const;
    };
}
