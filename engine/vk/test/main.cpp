#ifdef _WIN32

#include <windows.h>

#endif

#include <cstdlib>
#include <string>
#include "../VkVixen.h"
#include "VkPipeline.h"
#include "VkRenderer.h"

int main() {
#ifdef _WIN32
    system(("chcp " + std::to_string(CP_UTF8)).c_str());
#endif
    spdlog::set_level(spdlog::level::trace);

    auto vixen = Vixen::Vk::VkVixen("Vixen Vulkan Test", {1, 0, 0});

    auto vertex = Vixen::Vk::VkShaderModule::Builder()
            .setStage(Vixen::ShaderModule::Stage::VERTEX)
            .compileFromFile("../../editor/shaders/triangle.vert")
            .build(vixen.device);
    auto fragment = Vixen::Vk::VkShaderModule::Builder()
            .setStage(Vixen::ShaderModule::Stage::FRAGMENT)
            .compileFromFile("../../editor/shaders/triangle.frag")
            .build(vixen.device);
    auto program = Vixen::Vk::VkShaderProgram({vertex, fragment});

    auto pipeline = Vixen::Vk::VkPipeline::Builder()
            .setWidth(720)
            .setHeight(480)
            .build(vixen.device, vixen.swapchain, program);

    auto renderer = Vixen::Vk::VkRenderer(vixen.device, vixen.swapchain, pipeline);

    double old = glfwGetTime();
    uint32_t fps;
    while (!vixen.window.shouldClose()) {
        Vixen::Vk::VkWindow::update();

        renderer.render();

        fps++;
        double now = glfwGetTime();
        if (old - now >= 1) {
            spdlog::info("FPS: {}", fps);
            old = now;
            fps = 0;
        }
    }

    return EXIT_SUCCESS;
}
