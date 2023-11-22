#ifdef _WIN32

#include <windows.h>

#endif

#include <cstdlib>
#include <string>
#include "VkVixen.h"
#include "VkPipeline.h"
#include "VkRenderer.h"
#include "VkDescriptorPool.h"
#include "VkDescriptorSet.h"
#include "../../Camera.h"

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};

int main() {
#ifdef _WIN32
    system(("chcp " + std::to_string(CP_UTF8)).c_str());
#endif

#ifdef DEBUG
    spdlog::set_level(spdlog::level::trace);
#endif

    auto vixen = Vixen::Vk::VkVixen("Vixen Vulkan Test", {1, 0, 0});

    const auto vertex = Vixen::Vk::VkShaderModule::Builder(Vixen::ShaderModule::Stage::VERTEX)
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
                        .compileFromFile(vixen.device, "../../src/editor/shaders/triangle.vert");
    const auto fragment = Vixen::Vk::VkShaderModule::Builder(Vixen::ShaderModule::Stage::FRAGMENT)
        .compileFromFile(vixen.device, "../../src/editor/shaders/triangle.frag");
    const auto program = Vixen::Vk::VkShaderProgram(vertex, fragment);

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
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}
    };

    std::vector<uint32_t> indices{
        0, 1, 2,
        2, 3, 0
    };

    const auto buffer = Vixen::Vk::VkBuffer::stage(
        vixen.device,
        Vixen::Buffer::Usage::VERTEX |
        Vixen::Buffer::Usage::INDEX,
        vertices.size() * sizeof(Vertex) +
        indices.size() * sizeof(uint32_t),
        [&vertices, &indices](auto data) {
            memcpy(
                data,
                vertices.data(),
                sizeof(Vertex) * vertices.size()
            );

            memcpy(
                static_cast<Vertex*>(data) + vertices.size(),
                indices.data(),
                sizeof(uint32_t) * indices.size()
            );
        }
    );

    auto camera = Vixen::Camera({0.0f, 0.0f, -5.0f});

    const std::vector<VkDescriptorPoolSize> sizes{
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1
        }
    };

    auto uniformBuffer = Vixen::Vk::VkBuffer(
        vixen.device,
        Vixen::Buffer::Usage::UNIFORM,
        sizeof(UniformBufferObject)
    );

    UniformBufferObject ubo{
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        camera.view(),
        camera.perspective(static_cast<float>(width) / static_cast<float>(height))
    };

    auto descriptorPool = std::make_shared<Vixen::Vk::VkDescriptorPool>(vixen.device, sizes, 1);
    auto descriptorSet = Vixen::Vk::VkDescriptorSet(vixen.device, descriptorPool, vertex->getDescriptorSetLayout());
    descriptorSet.updateUniformBuffer(0, uniformBuffer);

    double old = glfwGetTime();
    double lastFrame = old;
    uint32_t fps = 0;
    while (!vixen.window.shouldClose()) {
        if (vixen.window.update()) {
            vixen.swapchain.invalidate();
            // TODO: Recreating the entire renderer is probably overkill, need a better way to recreate framebuffers on resize triggered from window
            renderer = std::make_unique<Vixen::Vk::VkRenderer>(vixen.device, vixen.swapchain, pipeline);
        }

        const double& now = glfwGetTime();
        double deltaTime = now - lastFrame;
        lastFrame = now;
        ubo.model = glm::rotate(ubo.model,  static_cast<float>(deltaTime) * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        uniformBuffer.write(reinterpret_cast<const char*>(&ubo), sizeof(UniformBufferObject), 0);

        renderer->render(buffer, vertices.size(), indices.size(), descriptorSet);

        fps++;
        if (now - old >= 1) {
            spdlog::info("FPS: {}", fps);
            old = now;
            fps = 0;
        }
    }

    vixen.device->waitIdle();

    return EXIT_SUCCESS;
}
