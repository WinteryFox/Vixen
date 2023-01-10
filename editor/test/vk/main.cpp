#ifdef _WIN32

#include <windows.h>

#endif

#include <cstdlib>
#include <string>
#include "vk/Window.h"
#include "vk/Instance.h"

using namespace Vixen::Engine::Vk;

int main() {
#ifdef _WIN32
    system(("chcp " + std::to_string(CP_UTF8)).c_str());
#endif
    spdlog::set_level(spdlog::level::trace);

    auto window = Window("Vixen Vulkan Test", 720, 480, false);
    window.center();
    window.setVisible(true);

    auto instance = std::make_shared<Instance>("Vixen Vk Test", glm::vec3(1, 0, 0), window.requiredExtensions);
    //auto surface = Surface(instance, window);

    while (!window.shouldClose()) {
        Vixen::Engine::Vk::Window::update();
    }
    return EXIT_SUCCESS;
}
