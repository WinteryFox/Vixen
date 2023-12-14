#ifdef _WIN32

#include <windows.h>

#endif

#include <cstdlib>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include "VkVixen.h"
#include "VkPipeline.h"
#include "VkRenderer.h"
#include "VkDescriptorPool.h"
#include "VkDescriptorSet.h"
#include "../../Camera.h"

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 uv;
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

    const auto vertexShader = Vixen::Vk::VkShaderModule::Builder(Vixen::ShaderModule::Stage::VERTEX)
                              .addBinding({
                                  .binding = 0,
                                  .stride = sizeof(Vertex),
                                  .rate = Vixen::Vk::VkShaderModule::Rate::VERTEX
                              })
                              .addInput({
                                  .binding = 0,
                                  .location = 0,
                                  .size = sizeof(glm::vec3),
                                  .offset = offsetof(Vertex, position)
                              })
                              .addInput({
                                  .binding = 0,
                                  .location = 1,
                                  .size = sizeof(glm::vec3),
                                  .offset = offsetof(Vertex, color)
                              })
                              .addInput({
                                  .binding = 0,
                                  .location = 2,
                                  .size = sizeof(glm::vec2),
                                  .offset = offsetof(Vertex, uv)
                              })
                              .compileFromFile(vixen.device, "../../src/editor/shaders/triangle.vert");
    const auto fragment = Vixen::Vk::VkShaderModule::Builder(Vixen::ShaderModule::Stage::FRAGMENT)
        .compileFromFile(vixen.device, "../../src/editor/shaders/triangle.frag");
    const auto program = Vixen::Vk::VkShaderProgram(vertexShader, fragment);

    int width;
    int height;
    vixen.window.getFramebufferSize(width, height);

    auto pipeline = Vixen::Vk::VkPipeline::Builder()
                    .setWidth(width)
                    .setHeight(height)
                    .build(vixen.device, vixen.swapchain, program);

    auto renderer = std::make_unique<Vixen::Vk::VkRenderer>(vixen.device, vixen.swapchain, pipeline);

    Assimp::Importer importer;
    const auto& scene = importer.ReadFile("../../src/engine/vk/test/vikingroom.glb",
                                          aiProcessPreset_TargetRealtime_Fast);
    if (!scene)
        throw std::runtime_error("Failed to load model from file");

    const auto& mesh = scene->mMeshes[0];
    const auto& hasColors = mesh->HasVertexColors(0);
    const auto& hasUvs = mesh->HasTextureCoords(0);

    std::vector<Vertex> vertices(mesh->mNumVertices);
    for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        const auto& vertex = mesh->mVertices[i];
        // TODO: Instead of storing default values for each vertex where a color or UV is missing, we should compact this down to save memory
        const auto& color = hasColors ? mesh->mColors[0][i] : aiColor4D{1.0f, 1.0f, 1.0f, 1.0f};
        const auto& uv = hasUvs ? mesh->mTextureCoords[0][i] : aiVector3D{1.0f, 1.0f, 1.0f};

        vertices[i] = Vertex{
            .position = {vertex.x, vertex.y, vertex.z},
            .color = {color.r, color.g, color.b},
            .uv = {uv.x, uv.y}
        };
    }

    std::vector<uint32_t> indices(mesh->mNumFaces * 3);
    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        const auto& face = mesh->mFaces[i];
        if (face.mNumIndices != 3) {
            spdlog::warn("Skipping face with {} indices", face.mNumIndices);
            continue;
        }

        indices[i * 3] = face.mIndices[0];
        indices[i * 3 + 1] = face.mIndices[1];
        indices[i * 3 + 2] = face.mIndices[2];
    }

    aiString path;
    scene->mMaterials[mesh->mMaterialIndex]->GetTexture(aiTextureType_DIFFUSE, 0, &path);

    const auto& texture = scene->GetEmbeddedTexture(path.C_Str());
    assert(texture != nullptr && "Texture is nullptr");

    std::shared_ptr<Vixen::Vk::VkImage> image;
    if (texture->mHeight != 0) {
        image = nullptr; // TODO
    }
    else {
        image = std::make_shared<Vixen::Vk::VkImage>(
            Vixen::Vk::VkImage::from(
                vixen.device,
                texture->achFormatHint,
                reinterpret_cast<const std::byte*>(texture->pcData),
                texture->mWidth
            )
        );
    }

    const auto buffer = Vixen::Vk::VkBuffer::stage(
        vixen.device,
        Vixen::Buffer::Usage::VERTEX |
        Vixen::Buffer::Usage::INDEX,
        vertices.size() * sizeof(Vertex) +
        indices.size() * sizeof(uint32_t),
        [&vertices, &indices](const auto& data) {
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

    auto camera = Vixen::Camera(glm::vec3{1.0f, 1.0f, 1.0f});

    const std::vector<VkDescriptorPoolSize> sizes{
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
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
    auto mvp = Vixen::Vk::VkDescriptorSet(vixen.device, descriptorPool, *program.getDescriptorSetLayout());
    mvp.updateUniformBuffer(0, uniformBuffer, 0, uniformBuffer.getSize());

    auto testImage = std::make_shared<Vixen::Vk::VkImage>(
        Vixen::Vk::VkImage::from(vixen.device, std::string("../../src/engine/vk/test/texture.jpg")));
    auto view = Vixen::Vk::VkImageView(image, VK_IMAGE_ASPECT_COLOR_BIT);
    auto sampler = Vixen::Vk::VkSampler(vixen.device);

    //auto albedo = Vixen::Vk::VkDescriptorSet(vixen.device, descriptorPool, *program.getDescriptorSetLayout());
    mvp.updateCombinedImageSampler(1, sampler, view);

    const std::vector descriptorSets = {mvp.getSet()};

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
        ubo.model = rotate(glm::mat4(1.0f), /*static_cast<float>(deltaTime) **/ glm::radians(90.0f),
                           glm::vec3(1.0f, 0.0f, 0.0f));
        ubo.view = camera.view();
        ubo.projection = camera.perspective(
            static_cast<float>(vixen.swapchain.getExtent().width) /
            static_cast<float>(vixen.swapchain.getExtent().height)
        );
        uniformBuffer.write(reinterpret_cast<const char*>(&ubo), sizeof(UniformBufferObject), 0);

        renderer->render(buffer, vertices.size(), indices.size(), descriptorSets);

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
