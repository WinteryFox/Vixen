#pragma once

#include "../Vixen.h"
#include "VkWindow.h"
#include "Instance.h"
#include "Device.h"
#include "Swapchain.h"
#include "VkShaderModule.h"

namespace Vixen::Vk {
    class VkVixen : public Vixen {
    public:
        const std::vector<const char *> deviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        VkWindow window;

        Instance instance;

        VkSurfaceKHR surface;

        std::shared_ptr<Device> device;

        Swapchain swapchain;

        VkVixen(const std::string &appTitle, glm::vec3 appVersion);

        VkVixen(const VkVixen &) = delete;

        VkVixen &operator=(const VkVixen &) = delete;
    };
}
