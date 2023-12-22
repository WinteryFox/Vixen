#ifdef _WIN32

#include <windows.h>

#endif

#include <cstdlib>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include "VkBuffer.h"
#include "VkDescriptorPool.h"
#include "VkDescriptorSet.h"
#include "VkPipeline.h"
#include "VkRenderer.h"
#include "VkVixen.h"
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
                              .compileFromFile(vixen.getDevice(), "../../src/editor/shaders/triangle.vert");
    const auto fragment = Vixen::Vk::VkShaderModule::Builder(Vixen::ShaderModule::Stage::FRAGMENT)
        .compileFromFile(vixen.getDevice(), "../../src/editor/shaders/triangle.frag");
    const auto program = Vixen::Vk::VkShaderProgram(vertexShader, fragment);

    int width;
    int height;
    vixen.getWindow()->getFramebufferSize(width, height);

    auto pipeline = Vixen::Vk::VkPipeline::Builder()
                    .setWidth(width)
                    .setHeight(height)
                    .setFormat(vixen.getSwapchain()->getFormat().format)
                    .build(vixen.getDevice(), program);

    auto renderer = std::make_unique<Vixen::Vk::VkRenderer>(pipeline, vixen.getSwapchain());

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
        // TODO: Not implemented
        throw std::runtime_error("Not implemented");
    }
    else {
        image = std::make_shared<Vixen::Vk::VkImage>(
            Vixen::Vk::VkImage::from(
                vixen.getDevice(),
                texture->achFormatHint,
                reinterpret_cast<std::byte*>(texture->pcData),
                texture->mWidth
            )
        );
    }

    auto stagingBuffer = Vixen::Vk::VkBuffer(
        vixen.getDevice(),
        Vixen::Buffer::Usage::VERTEX |
        Vixen::Buffer::Usage::INDEX |
        Vixen::Buffer::Usage::TRANSFER_SRC,
        vertices.size() * sizeof(Vertex) +
        indices.size() * sizeof(uint32_t)
    );
    const auto& buffer = Vixen::Vk::VkBuffer(
        vixen.getDevice(),
        Vixen::Buffer::Usage::VERTEX |
        Vixen::Buffer::Usage::INDEX |
        Vixen::Buffer::Usage::TRANSFER_DST,
        vertices.size() * sizeof(Vertex) +
        indices.size() * sizeof(uint32_t)
    );

    stagingBuffer.write(
        reinterpret_cast<std::byte*>(vertices.data()),
        vertices.size() * sizeof(vertices[0]),
        0
    );
    stagingBuffer.write(
        reinterpret_cast<std::byte*>(indices.data()),
        indices.size() * sizeof(indices[0]),
        vertices.size() * sizeof(vertices[0])
    );

    vixen.getDevice()
         ->getTransferCommandPool()
         ->allocate(Vixen::Vk::VkCommandBuffer::Level::PRIMARY)
         .begin(Vixen::Vk::VkCommandBuffer::Usage::SINGLE)
         .copyBuffer(stagingBuffer, buffer)
         .end()
         .submit(vixen.getDevice()->getTransferQueue(), {}, {}, {});

    auto camera = Vixen::Camera(glm::vec3{0.0f, 0.0f, 0.0f});

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
        vixen.getDevice(),
        Vixen::Buffer::Usage::UNIFORM,
        sizeof(UniformBufferObject)
    );

    UniformBufferObject ubo{
        glm::mat4(1.0f),
        camera.view(),
        camera.perspective(static_cast<float>(width) / static_cast<float>(height))
    };
    ubo.model = translate(ubo.model, {0.0f, -0.5f, -1.5f});
    //ubo.model = rotate(ubo.model, glm::radians(45.0f), {1.0f, 0.0f, 1.0f});
    ubo.model = rotate(ubo.model, glm::radians(225.0f), {0.0f, 1.0f, 0.0f});
    //ubo.model = rotate(ubo.model, glm::radians(45.0f), {0.0f, 0.0f, 1.0f});

    auto descriptorPool = std::make_shared<Vixen::Vk::VkDescriptorPool>(vixen.getDevice(), sizes, 1);
    auto mvp = Vixen::Vk::VkDescriptorSet(vixen.getDevice(), descriptorPool, *program.getDescriptorSetLayout());
    mvp.updateUniformBuffer(0, uniformBuffer, 0, uniformBuffer.getSize());

    auto view = Vixen::Vk::VkImageView(image, VK_IMAGE_ASPECT_COLOR_BIT);
    auto sampler = Vixen::Vk::VkSampler(vixen.getDevice());
    mvp.updateCombinedImageSampler(1, sampler, view);

    const std::vector descriptorSets = {mvp.getSet()};

    double old = glfwGetTime();
    double lastFrame = old;
    uint32_t fps = 0;
    while (!vixen.getWindow()->shouldClose()) {
        if (vixen.getWindow()->update()) {
            vixen.getSwapchain()->invalidate();
            // TODO: Recreating the entire renderer is probably overkill, need a better way to recreate framebuffers on resize triggered from window
            renderer = std::make_unique<Vixen::Vk::VkRenderer>(pipeline, vixen.getSwapchain());
        }

        const double& now = glfwGetTime();
        double deltaTime = now - lastFrame;
        lastFrame = now;
        ubo.view = camera.view();
        const auto& [width, height] = vixen.getSwapchain()->getExtent();
        ubo.projection = camera.perspective(
            static_cast<float>(width) /
            static_cast<float>(height)
        );
        uniformBuffer.write(reinterpret_cast<std::byte*>(&ubo), sizeof(UniformBufferObject), 0);

        renderer->render(buffer, vertices.size(), indices.size(), descriptorSets);

        fps++;
        if (now - old >= 1) {
            spdlog::info("FPS: {}", fps);
            old = now;
            fps = 0;
        }
    }

    vixen.getDevice()->waitIdle();

    return EXIT_SUCCESS;
}
