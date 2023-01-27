#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#include "VkVixen.h"

namespace Vixen::Engine {
    VkVixen::VkVixen(const std::string &appTitle, glm::vec3 appVersion)
            : window(VkWindow(appTitle, 720, 480, false)),
              instance(Instance(appTitle, appVersion, window.requiredExtensions)),
              device(Device(
                      deviceExtensions,
                      instance.findOptimalGraphicsCard(deviceExtensions),
                      instance.surfaceForWindow(window)
              )),
              allocator(std::make_shared<Allocator>(device.gpu.device, device.device, instance.instance)) {
        window.center();
        window.setVisible(true);
    }
}
