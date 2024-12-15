#ifdef _WIN32

#include <Windows.h>

#endif

#include <VulkanApplication.h>
#include <core/Camera.h>

#include "device/VulkanDevice.h"

int main() {
#ifdef _WIN32
    system(std::format("chcp {}", CP_UTF8).c_str());
#endif

#ifdef DEBUG
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("%Y-%m-%d %T.%e %^%7l%$ %P --- [%t] %1v");
#endif

    try {
        auto vixen = Vixen::VulkanApplication("Vixen Vulkan Test", {1, 0, 0});

        vixen.update();
        vixen.run();
    } catch (const std::runtime_error &e) {
        spdlog::error(e.what());
        throw;
    }

    return EXIT_SUCCESS;
}
