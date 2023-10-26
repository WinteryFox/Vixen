#ifdef _WIN32

#include <windows.h>

#endif

#include <cstdlib>
#include <string>
#include "VkVixen.h"
#include "VkPipeline.h"
#include "VkRenderer.h"

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
};

int main() {
#ifdef _WIN32
    system(("chcp " + std::to_string(CP_UTF8)).c_str());
#endif
    spdlog::set_level(spdlog::level::trace);

    auto vixen = Vixen::Vk::VkVixen("Vixen Vulkan Test", {1, 0, 0});

    auto vertex = Vixen::Vk::VkShaderModule::Builder()
            .setStage(Vixen::ShaderModule::Stage::VERTEX)
            .addBinding({
                .binding = 0,
                .stride = sizeof(Vertex),
                .rate = Vixen::Vk::VkShaderModule::Rate::VERTEX,
            })
            .addInput({
                .binding = 0,
                .location = 0,
                .offset = offsetof(Vertex, position),
            })
            .addInput({
                .binding = 0,
                .location = 1,
                .offset = offsetof(Vertex, color),
            })
            .compileFromFile(vixen.device, "../../editor/shaders/triangle.vert");
    auto fragment = Vixen::Vk::VkShaderModule::Builder()
            .setStage(Vixen::ShaderModule::Stage::FRAGMENT)
            .compileFromFile(vixen.device, "../../editor/shaders/triangle.frag");
    auto program = Vixen::Vk::VkShaderProgram(vertex, fragment);

    int width;
    int height;
    vixen.window.getFramebufferSize(width, height);

    auto pipeline = Vixen::Vk::VkPipeline::Builder()
            .setWidth(width)
            .setHeight(height)
            .build(vixen.device, vixen.swapchain, program);

    auto renderer = std::make_unique<Vixen::Vk::VkRenderer>(vixen.device, vixen.swapchain, pipeline);

    std::vector<Vertex> vertices{
            {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f,  -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f,  0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}}
    };

    std::vector<uint32_t> indices{
            0, 1, 2,
            2, 3, 0
    };

    auto buffer = Vixen::Vk::VkBuffer::stage(
            vixen.device,
            Vixen::Vk::Buffer::Usage::VERTEX |
            Vixen::Vk::Buffer::Usage::INDEX,
            vertices.size() * sizeof(Vertex) +
            indices.size() * sizeof(uint32_t),
            [&vertices, &indices](auto data) {
                memcpy(
                        data,
                        vertices.data(),
                        sizeof(Vertex) * vertices.size()
                );

                memcpy(
                        static_cast<Vertex *>(data) + vertices.size(),
                        indices.data(),
                        sizeof(uint32_t) * indices.size()
                );
            }
    );

    double old = glfwGetTime();
    uint32_t fps;
    while (!vixen.window.shouldClose()) {
        if (vixen.window.update()) {
            vixen.swapchain.invalidate();
            // TODO: Recreating the entire renderer is probably overkill, need a better way to recreate framebuffers on resize triggered from window
            renderer = std::make_unique<Vixen::Vk::VkRenderer>(vixen.device, vixen.swapchain, pipeline);
        }

        renderer->render(buffer, vertices.size(), indices.size());

        fps++;
        double now = glfwGetTime();
        if (now - old >= 1) {
            spdlog::info("FPS: {}", fps);
            old = now;
            fps = 0;
        }
    }

    vixen.device->waitIdle();

    return EXIT_SUCCESS;
}
