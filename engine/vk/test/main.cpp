#ifdef _WIN32

#include <windows.h>

#endif

#include <cstdlib>
#include <string>
#include "../VkVixen.h"

int main() {
#ifdef _WIN32
    system(("chcp " + std::to_string(CP_UTF8)).c_str());
#endif
    spdlog::set_level(spdlog::level::trace);

    auto vixen = Vixen::Engine::VkVixen("Vixen Vulkan Test", {1, 0, 0});
    auto vertex = Vixen::Engine::VkShaderModule::Builder()
            .compileFromFile("../../editor/shaders/triangle.vert")
            .setStage(Vixen::Engine::ShaderModule::Stage::VERTEX)
            .build(vixen.device);
    auto fragment = Vixen::Engine::VkShaderModule::Builder()
            .compileFromFile("../../editor/shaders/triangle.frag")
            .setStage(Vixen::Engine::ShaderModule::Stage::FRAGMENT)
            .build(vixen.device);

    while (!vixen.window.shouldClose()) {
        Vixen::Engine::VkWindow::update();
    }
    return EXIT_SUCCESS;
}
