#pragma once

#include <memory>
#include <set>
#include "Instance.h"

namespace Vixen::Vk {
    class Device {
        GraphicsCard gpu;

        VkDevice device;

        VkSurfaceKHR surface;

        QueueFamily graphicsQueueFamily;

        VkQueue graphicsQueue;

        QueueFamily presentQueueFamily;

        VkQueue presentQueue;

    public:
        Device(const std::vector<const char *> &extensions, GraphicsCard gpu, VkSurfaceKHR surface);

        ~Device();

        const VkDevice getDevice() const;

        const GraphicsCard &getGpu() const;

        const VkSurfaceKHR getSurface() const;

        const VkQueue_T *getGraphicsQueue() const;

        const VkQueue_T *getPresentQueue() const;

        const QueueFamily &getGraphicsQueueFamily() const;

        const QueueFamily &getPresentQueueFamily() const;

        [[nodiscard]] VkQueue getQueueHandle(uint32_t queueFamilyIndex, uint32_t queueIndex = 0) const;
    };
}
