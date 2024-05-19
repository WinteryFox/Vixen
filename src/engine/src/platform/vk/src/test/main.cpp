#ifdef _WIN32

#include <windows.h>

#endif

#include <Camera.h>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <src/VulkanApplication.h>
#include <src/VulkanWindow.h>

#include "PrimitiveTopology.h"
#include "src/VulkanMesh.h"
#include "src/Renderer.h"
#include "src/VulkanSwapchain.h"
#include "src/descriptorset/VulkanDescriptorPoolExpanding.h"
#include "src/image/VulkanImage.h"
#include "src/image/VulkanImageView.h"
#include "src/material/Material.h"
#include "src/material/MaterialPass.h"
#include "src/pipeline/VulkanPipeline.h"
#include "src/shader/VulkanShaderModule.h"
#include "src/shader/VulkanShaderProgram.h"

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
    spdlog::set_pattern("%Y-%m-%d %T.%e %^%7l%$ %P --- [%t] %1v");
#endif

    auto vixen = Vixen::VulkanApplication("Vixen Vulkan Test", {1, 0, 0});

    const auto vertexShader = Vixen::VulkanShaderModule::Builder(Vixen::VulkanShaderModule::Stage::Vertex)
                              .addBinding({
                                  .binding = 0,
                                  .stride = sizeof(Vixen::Vertex),
                                  .rate = Vixen::VulkanShaderModule::Rate::Vertex
                              })
                              .addInput({
                                  .binding = 0,
                                  .location = 0,
                                  .size = sizeof(glm::vec3),
                                  .offset = offsetof(Vixen::Vertex, position)
                              })
                              .addInput({
                                  .binding = 0,
                                  .location = 1,
                                  .size = sizeof(glm::vec4),
                                  .offset = offsetof(Vixen::Vertex, color)
                              })
                              .addInput({
                                  .binding = 0,
                                  .location = 2,
                                  .size = sizeof(glm::vec2),
                                  .offset = offsetof(Vixen::Vertex, uv)
                              })
                              .compileFromFile(vixen.getDevice(), "../../src/editor/src/shaders/triangle.vert");
    const auto fragment = Vixen::VulkanShaderModule::Builder(Vixen::VulkanShaderModule::Stage::Fragment)
        .compileFromFile(vixen.getDevice(), "../../src/editor/src/shaders/triangle.frag");
    const auto program = Vixen::VulkanShaderProgram(vertexShader, fragment);

    int width;
    int height;
    vixen.getWindow()->getFramebufferSize(width, height);

    auto pipeline = Vixen::VulkanPipeline::Builder()
                    .setWidth(width)
                    .setHeight(height)
                    .setColorFormat(vixen.getSwapchain()->getColorFormat().format)
                    .setDepthFormat(vixen.getSwapchain()->getDepthFormat())
                    .build(vixen.getDevice(), program);

    auto renderer = std::make_unique<Vixen::Renderer>(pipeline, vixen.getSwapchain());

    const std::string& file = "../../src/editor/models/sponza/Sponza.gltf";
    const std::string& path = std::filesystem::path(file).remove_filename().string();

    std::vector<Vixen::VulkanDescriptorPoolExpanding::PoolSizeRatio> ratios = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
    };
    auto descriptorPool = std::make_shared<Vixen::VulkanDescriptorPoolExpanding>(vixen.getDevice(), 1000, ratios);

    auto camera = Vixen::Camera(glm::vec3{0.0f, 0.0f, 0.0f});

    auto uniformBuffer = Vixen::VulkanBuffer(
        vixen.getDevice(),
        Vixen::BufferUsage::Uniform,
        1,
        sizeof(UniformBufferObject)
    );

    UniformBufferObject ubo{
        glm::mat4(1.0F),
        camera.view(),
        camera.perspective(static_cast<float>(width) / static_cast<float>(height))
    };
    ubo.model = scale(ubo.model, {0.1F, 0.1F, 0.1F});

    Assimp::Importer importer;
    const auto& scene = importer.ReadFile(file, aiProcessPreset_TargetRealtime_Fast);
    if (!scene)
        throw std::runtime_error("Failed to load model from file");

    std::vector<Vixen::VulkanMesh> meshes{};
    meshes.reserve(scene->mNumMeshes);

    for (auto i = 0; i < scene->mNumMeshes; i++) {
        const auto& aiMesh = scene->mMeshes[i];
        const auto& hasColors = aiMesh->HasVertexColors(0);
        const auto& hasUvs = aiMesh->HasTextureCoords(0);

        std::vector<Vixen::Vertex> vertices(aiMesh->mNumVertices);
        for (uint32_t j = 0; j < aiMesh->mNumVertices; j++) {
            const auto& vertex = aiMesh->mVertices[j];
            // TODO: Instead of storing default values for each vertex where a color or UV is missing, we should compact this down to save memory
            const auto& color = hasColors ? aiMesh->mColors[0][j] : aiColor4D{1.0F, 1.0F, 1.0F, 1.0F};
            const auto& textureCoord = hasUvs ? aiMesh->mTextureCoords[0][j] : aiVector3D{1.0F, 1.0F, 1.0F};

            vertices[j] = Vixen::Vertex{
                .position = {vertex.x, vertex.y, vertex.z},
                .color = {color.r, color.g, color.b, color.a},
                .uv = {textureCoord.x, textureCoord.y}
            };
        }

        std::vector<uint32_t> indices(aiMesh->mNumFaces * 3);
        for (uint32_t j = 0; j < aiMesh->mNumFaces; j++) {
            const auto& face = aiMesh->mFaces[j];
            if (face.mNumIndices != 3) {
                spdlog::warn("Skipping face with {} indices", face.mNumIndices);
                continue;
            }

            indices[j * 3] = face.mIndices[0];
            indices[j * 3 + 1] = face.mIndices[1];
            indices[j * 3 + 2] = face.mIndices[2];
        }

        aiString imagePath;
        const auto& aiMaterial = scene->mMaterials[scene->mMeshes[i]->mMaterialIndex];
        if (aiMaterial == nullptr)
            throw std::runtime_error("Material is nullptr");

        aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &imagePath);
        const auto& texture = scene->GetEmbeddedTexture(imagePath.C_Str());

        std::shared_ptr<Vixen::VulkanImage> image;
        if (texture == nullptr) {
            image = std::make_shared<Vixen::VulkanImage>(
                Vixen::VulkanImage::from(
                    vixen.getDevice(),
                    path + imagePath.C_Str()
                )
            );
        } else {
            image = std::make_shared<Vixen::VulkanImage>(
                Vixen::VulkanImage::from(
                    vixen.getDevice(),
                    reinterpret_cast<std::byte*>(texture->pcData),
                    texture->mWidth
                )
            );
        }

        auto descriptor = descriptorPool->allocate(*program.getDescriptorSetLayout());
        descriptor->writeUniformBuffer(0, uniformBuffer, 0, uniformBuffer.getSize());

        auto imageView = std::make_shared<Vixen::VulkanImageView>(vixen.getDevice(), image, VK_IMAGE_ASPECT_COLOR_BIT);
        descriptor->writeCombinedImageSampler(1, *imageView);

        auto material = std::make_shared<Vixen::Material>(
            pipeline,
            image,
            imageView,
            descriptor,
            Vixen::MaterialPass::Opaque
        );

        meshes.emplace_back(vixen.getDevice());
        meshes[i].setVertices(vertices);
        meshes[i].setIndices(indices, Vixen::PrimitiveTopology::TriangleList);
        meshes[i].setMaterial(material);
    }

    double old = glfwGetTime();
    double lastFrame = old;
    uint32_t fps = 0;
    while (!vixen.getWindow()->shouldClose()) {
        if (vixen.getWindow()->update()) {
            vixen.getSwapchain()->invalidate();
            // TODO: Recreating the entire renderer is probably overkill, need a better way to recreate framebuffers on resize triggered from window
            renderer = std::make_unique<Vixen::Renderer>(pipeline, vixen.getSwapchain());
        }

        const double& now = glfwGetTime();
        double deltaTime = now - lastFrame;
        camera.update(vixen.getWindow()->getWindow(), deltaTime);

        lastFrame = now;
        ubo.view = camera.view();
        const auto& [width, height] = vixen.getSwapchain()->getExtent();
        ubo.projection = camera.perspective(
            static_cast<float>(width) /
            static_cast<float>(height)
        );
        uniformBuffer.setData(reinterpret_cast<const std::byte*>(&ubo));

        renderer->render(meshes);

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
