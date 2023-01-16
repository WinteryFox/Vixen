#ifdef _WIN32

#include <windows.h>

#endif

#include <cstdlib>
#include <string>
#include "vk/VkWindow.h"
#include "vk/Instance.h"
#include "vk/Device.h"

using namespace Vixen::Engine;

int main() {
#ifdef _WIN32
    system(("chcp " + std::to_string(CP_UTF8)).c_str());
#endif
    spdlog::set_level(spdlog::level::trace);

    auto window = VkWindow("Vixen Vulkan Test", 720, 480, false);
    window.center();
    window.setVisible(true);

    auto instance = Instance("Vixen Vk Test", glm::vec3(1, 0, 0), window.requiredExtensions);
    auto device = Device(instance.findOptimalGraphicsCard(), instance.surfaceForWindow(window));

    while (!window.shouldClose()) {
        VkWindow::update();
    }
    return EXIT_SUCCESS;
}
