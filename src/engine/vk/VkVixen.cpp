#include "VkVixen.h"

namespace Vixen::Vk {
    VkVixen::VkVixen(const std::string& appTitle, glm::vec3 appVersion)
        : Vixen(appTitle, appVersion),
          window(std::make_unique<VkWindow>(appTitle, 640, 480, false)),
          instance(Instance(appTitle, appVersion, window->getRequiredExtensions())),
          surface(instance.surfaceForWindow(*window)),
          device(std::make_shared<Device>(
              instance,
              deviceExtensions,
              instance.findOptimalGraphicsCard(surface, deviceExtensions),
              surface
          )),
          swapchain(std::make_shared<Swapchain>(device, 3)) {
        window->center();
        window->setVisible(true);
    }

    std::vector<const char*> VkVixen::getDeviceExtensions() const { return deviceExtensions; }

    std::shared_ptr<VkWindow> VkVixen::getWindow() const { return window; }

    Instance VkVixen::getInstance() const { return instance; }

    VkSurfaceKHR VkVixen::getSurface() const { return surface; }

    std::shared_ptr<Device> VkVixen::getDevice() const { return device; }

    std::shared_ptr<Swapchain> VkVixen::getSwapchain() const { return swapchain; }
}
