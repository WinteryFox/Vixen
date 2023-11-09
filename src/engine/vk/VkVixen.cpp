#include "VkVixen.h"

namespace Vixen::Vk {
    VkVixen::VkVixen(const std::string &appTitle, glm::vec3 appVersion)
            : Vixen(appTitle, appVersion),
              window(VkWindow(appTitle, 1920, 1080, false)),
              instance(Instance(appTitle, appVersion, window.requiredExtensions)),
              surface(instance.surfaceForWindow(window)),
              device(std::make_shared<Device>(
                      instance,
                      deviceExtensions,
                      instance.findOptimalGraphicsCard(surface, deviceExtensions),
                      surface
              )),
              swapchain(device, 3) {
        window.center();
        window.setVisible(true);
    }
}
