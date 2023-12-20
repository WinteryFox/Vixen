#pragma once

#include "../Vixen.h"
#include "VkWindow.h"
#include "Instance.h"
#include "Device.h"
#include "Swapchain.h"
#include "VkShaderModule.h"

namespace Vixen::Vk {
    class VkVixen : public Vixen {
        const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        std::shared_ptr<VkWindow> window;

        Instance instance;

        VkSurfaceKHR surface;

        std::shared_ptr<Device> device;

        std::shared_ptr<Swapchain> swapchain;

    public:
        VkVixen(const std::string& appTitle, glm::vec3 appVersion);

        VkVixen(const VkVixen&) = delete;

        VkVixen& operator=(const VkVixen&) = delete;

        [[nodiscard]] std::vector<const char*> getDeviceExtensions() const;

        [[nodiscard]] std::shared_ptr<VkWindow> getWindow() const;

        [[nodiscard]] Instance getInstance() const;

        [[nodiscard]] VkSurfaceKHR getSurface() const;

        [[nodiscard]] std::shared_ptr<Device> getDevice() const;

        [[nodiscard]] std::shared_ptr<Swapchain> getSwapchain() const;
    };
}
