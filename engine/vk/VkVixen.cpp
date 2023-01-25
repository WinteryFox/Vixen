#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VOLK_IMPLEMENTATION

#include "VkVixen.h"

namespace Vixen::Engine {
    VkVixen::VkVixen(const std::string &appTitle) {
        window = std::make_unique<VkWindow>(appTitle, 720, 480, false);
        window->center();
        window->setVisible(true);

        instance = std::make_unique<Instance>(appTitle, glm::vec3(1, 0, 0), window->requiredExtensions);

        std::vector<const char *> deviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        device = std::make_unique<Device>(
                deviceExtensions,
                instance->findOptimalGraphicsCard(deviceExtensions),
                instance->surfaceForWindow(window.get())
        );
    }
}
