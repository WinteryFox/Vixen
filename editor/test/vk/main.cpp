#ifdef _WIN32

#include <windows.h>

#endif

#include <cstdlib>
#include <string>
#include "vk/VkWindow.h"
#include "vk/Instance.h"

using namespace Vixen::Engine::Vk;

int main() {
#ifdef _WIN32
    system(("chcp " + std::to_string(CP_UTF8)).c_str());
#endif
    spdlog::set_level(spdlog::level::trace);

    auto window = VkWindow("Vixen Vulkan Test", 720, 480, false);
    window.center();
    window.setVisible(true);

    auto instance = Instance("Vixen Vk Test", glm::vec3(1, 0, 0), window.requiredExtensions);
    auto surface = instance.surfaceForWindow(window);

    while (!window.shouldClose()) {
        Vixen::Engine::Vk::VkWindow::update();
    }
    return EXIT_SUCCESS;
}
