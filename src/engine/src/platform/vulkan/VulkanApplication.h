#pragma once

#include <core/Application.h>

#include "Vulkan.h"

namespace Vixen {
    class VulkanSwapchain;
}

namespace Vixen {
    class VulkanDevice;
    class VulkanWindow;
    class Instance;

    class VulkanApplication : public Application {
        const std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        std::shared_ptr<VulkanWindow> window;

        std::shared_ptr<Instance> instance;

        VkSurfaceKHR surface;

        std::shared_ptr<VulkanDevice> device;

        std::shared_ptr<VulkanSwapchain> swapchain;

    public:
        VulkanApplication(const std::string &appTitle, glm::vec3 appVersion);

        VulkanApplication(const VulkanApplication &) = delete;

        VulkanApplication &operator=(const VulkanApplication &) = delete;

        [[nodiscard]] std::vector<const char *> getDeviceExtensions() const;

        [[nodiscard]] std::shared_ptr<VulkanWindow> getWindow() const;

        [[nodiscard]] std::shared_ptr<Instance> getInstance() const;

        [[nodiscard]] VkSurfaceKHR getSurface() const;

        [[nodiscard]] std::shared_ptr<VulkanDevice> getDevice() const;

        [[nodiscard]] std::shared_ptr<VulkanSwapchain> getSwapchain() const;
    };
}
