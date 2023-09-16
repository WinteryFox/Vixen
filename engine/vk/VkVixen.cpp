#include "VkVixen.h"

namespace Vixen::Engine {
    VkVixen::VkVixen(const std::string &appTitle, glm::vec3 appVersion)
            : Vixen(appTitle, appVersion),
              window(VkWindow(appTitle, 720, 480, false)),
              instance(Instance(appTitle, appVersion, window.requiredExtensions)),
              surface(instance.surfaceForWindow(window)),
              device(std::make_shared<Device>(
                      deviceExtensions,
                      instance.findOptimalGraphicsCard(surface, deviceExtensions),
                      surface
              )),
              allocator(std::make_shared<Allocator>(device->getGpu().device, device->getDevice(), instance.instance)),
              swapchain(device, VkSwapchain::FramesInFlight::TRIPLE_BUFFER) {
        window.center();
        window.setVisible(true);
    }
}
