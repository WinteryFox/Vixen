#ifdef _WIN32

#include <windows.h>

#endif

#include <cstdlib>
#include <string>
#include "../VkVixen.h"
#include "VkPipeline.h"

int main() {
#ifdef _WIN32
    system(("chcp " + std::to_string(CP_UTF8)).c_str());
#endif
    spdlog::set_level(spdlog::level::trace);

    auto vixen = Vixen::Vk::VkVixen("Vixen Vulkan Test", {1, 0, 0});

    auto vertex = Vixen::Vk::VkShaderModule::Builder()
            .compileFromFile("../../editor/shaders/triangle.vert")
            .setStage(Vixen::ShaderModule::Stage::VERTEX)
            .build(vixen.device);
    auto fragment = Vixen::Vk::VkShaderModule::Builder()
            .compileFromFile("../../editor/shaders/triangle.frag")
            .setStage(Vixen::ShaderModule::Stage::FRAGMENT)
            .build(vixen.device);
    auto program = Vixen::Vk::VkShaderProgram({vertex, fragment});

    auto pipeline = Vixen::Vk::VkPipeline::Builder()
            .setWidth(720)
            .setHeight(480)
            .build(vixen.device, vixen.swapchain, program);

    while (!vixen.window.shouldClose()) {
        Vixen::Vk::VkWindow::update();
    }
    return EXIT_SUCCESS;
}
