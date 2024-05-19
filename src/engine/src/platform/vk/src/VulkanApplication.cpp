#include "VulkanApplication.h"

#include "Instance.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"

namespace Vixen {
    VulkanApplication::VulkanApplication(const std::string& appTitle, glm::vec3 appVersion)
        : Application(appTitle, appVersion),
          window(std::make_unique<VulkanWindow>(appTitle, 640, 480, false)),
          instance(std::make_shared<Instance>(appTitle, appVersion, window->getRequiredExtensions())),
          surface(instance->surfaceForWindow(*window)),
          device(std::make_shared<VulkanDevice>(
              instance,
              deviceExtensions,
              instance->findOptimalGraphicsCard(surface, deviceExtensions),
              surface
          )),
          swapchain(std::make_shared<VulkanSwapchain>(device, 3)) {
        window->center();
        window->setVisible(true);
    }

    std::vector<const char*> VulkanApplication::getDeviceExtensions() const { return deviceExtensions; }

    std::shared_ptr<VulkanWindow> VulkanApplication::getWindow() const { return window; }

    std::shared_ptr<Instance> VulkanApplication::getInstance() const { return instance; }

    VkSurfaceKHR VulkanApplication::getSurface() const { return surface; }

    std::shared_ptr<VulkanDevice> VulkanApplication::getDevice() const { return device; }

    std::shared_ptr<VulkanSwapchain> VulkanApplication::getSwapchain() const { return swapchain; }
}
