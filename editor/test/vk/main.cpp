#ifdef _WIN32

#include <windows.h>

#endif

#include <cstdlib>
#include <string>
#include "vk/VkVixen.h"

using namespace Vixen::Engine;

int main() {
#ifdef _WIN32
    system(("chcp " + std::to_string(CP_UTF8)).c_str());
#endif
    spdlog::set_level(spdlog::level::trace);

    auto vixen = VkVixen("Vixen Vulkan Test", {1, 0, 0});

    while (!vixen.window.shouldClose()) {
        VkWindow::update();
    }
    return EXIT_SUCCESS;
}
