#pragma once

#include <memory>
#include <set>

#define VOLK_IMPLEMENTATION

#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>
#include "Instance.h"

namespace Vixen::Engine {
    class Device {
        GraphicsCard gpu;

        VkDevice device;

        VkSurfaceKHR surface;

        VkQueue graphicsQueue;

        VkQueue presentQueue;

    public:
        Device(const std::vector<const char *> &extensions, GraphicsCard gpu, VkSurfaceKHR surface);

        ~Device();

        [[nodiscard]] VkQueue getQueueHandle(uint32_t queueFamilyIndex, uint32_t queueIndex = 0) const;
    };
}
