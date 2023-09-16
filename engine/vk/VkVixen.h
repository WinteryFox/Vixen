#pragma once

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include "../Vixen.h"
#include "VkWindow.h"
#include "Instance.h"
#include "Device.h"
#include "Allocator.h"
#include "VkSwapchain.h"

namespace Vixen::Engine {
    class VkVixen : public Vixen {
    public:
        const std::vector<const char *> deviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        VkWindow window;

        Instance instance;

        VkSurfaceKHR surface;

        std::shared_ptr<Device> device;

        VkSwapchain swapchain;

        std::shared_ptr<Allocator> allocator;

        VkVixen(const std::string &appTitle, glm::vec3 appVersion);

        VkVixen(const VkVixen &) = delete;

        VkVixen &operator=(const VkVixen &) = delete;
    };
}
