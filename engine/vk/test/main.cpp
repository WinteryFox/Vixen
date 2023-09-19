#ifdef _WIN32

#include <windows.h>

#endif

#include <cstdlib>
#include <string>
#include "../VkVixen.h"
#include "Pipeline.h"

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
    auto program = Vixen::Engine::VkShaderProgram({vertex, fragment});

    auto pipeline = Vixen::Engine::Pipeline::Builder()
            .build(vixen.device, program);

    while (!vixen.window.shouldClose()) {
        Vixen::Engine::VkWindow::update();
    }
    return EXIT_SUCCESS;
}
