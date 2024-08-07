#include "VulkanApplication.h"

#include <assimp/postprocess.h>
#include <core/Camera.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <filesystem>
#include <glm/glm.hpp>

#include "buffer/VulkanBuffer.h"
#include "core/BufferUsage.h"
#include "core/PrimitiveTopology.h"
#include "descriptorset/VulkanDescriptorPoolExpanding.h"
#include "device/Instance.h"
#include "device/VulkanDevice.h"
#include "image/VulkanImage.h"
#include "image/VulkanImageView.h"
#include "material/Material.h"
#include "material/MaterialPass.h"
#include "pipeline/VulkanPipeline.h"
#include "rendering/Renderer.h"
#include "rendering/VulkanMesh.h"
#include "window/VulkanSwapchain.h"
#include "window/VulkanWindow.h"

namespace Vixen {
    VulkanApplication::VulkanApplication(const std::string &appTitle, glm::vec3 appVersion)
        : Application(appTitle, appVersion),
          window(std::make_unique<VulkanWindow>(appTitle, 720, 480, false)),
          instance(std::make_shared<Instance>(appTitle, appVersion, window->getRequiredExtensions())),
          surface(instance->surfaceForWindow(*window)),
          device(std::make_shared<VulkanDevice>(
              instance,
              deviceExtensions,
              instance->findOptimalGraphicsCard(surface, deviceExtensions),
              surface
          )),
          swapchain(std::make_shared<VulkanSwapchain>(device, 3)),
          pbrOpaqueShader(
              VulkanShaderModule::Builder(ShaderResources::Stage::Vertex)
              .compileFromFile(device, "../../src/editor/resources/shaders/pbr.vertex.glsl"),
              VulkanShaderModule::Builder(ShaderResources::Stage::Fragment)
              .compileFromFile(device, "../../src/editor/resources/shaders/pbr.fragment.glsl")
          ) {
        int width;
        int height;
        window->getFramebufferSize(width, height);

        pipeline = VulkanPipeline::Builder()
                .addBinding({
                    .binding = 0,
                    .stride = sizeof(Vertex),
                    .rate = ShaderResources::Rate::Vertex
                })
                .addInput({
                    .binding = 0,
                    .location = 0,
                    .type = ShaderResources::PrimitiveType::Float3,
                    .offset = offsetof(Vixen::Vertex, position)
                })
                .addInput({
                    .binding = 0,
                    .location = 1,
                    .type = ShaderResources::PrimitiveType::Float4,
                    .offset = offsetof(Vixen::Vertex, color)
                })
                .addInput({
                    .binding = 0,
                    .location = 2,
                    .type = ShaderResources::PrimitiveType::Float2,
                    .offset = offsetof(Vixen::Vertex, uv)
                })
                .addInput({
                    .binding = 0,
                    .location = 3,
                    .type = ShaderResources::PrimitiveType::Float3,
                    .offset = offsetof(Vixen::Vertex, normal)
                })
                .setWidth(width)
                .setHeight(height)
                .setColorFormat(swapchain->getColorFormat().format)
                .setDepthFormat(swapchain->getDepthFormat())
                .setMultisample({
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .rasterizationSamples = VK_SAMPLE_COUNT_8_BIT,
                    .sampleShadingEnable = VK_FALSE,
                    .minSampleShading = 1,
                    .pSampleMask = nullptr,
                    .alphaToCoverageEnable = VK_FALSE,
                    .alphaToOneEnable = VK_FALSE
                })
                .build(device, pbrOpaqueShader);

        renderer = std::make_unique<Renderer>(pipeline, swapchain);

        window->center();
        window->setVisible(true);
    }

    bool VulkanApplication::isRunning() const {
        return window->shouldClose();
    }

    void VulkanApplication::update() {
        if (window->update()) {
            swapchain->invalidate();
            // TODO: Recreating the entire renderer is probably overkill, need a better way to recreate framebuffers on resize triggered from window
            renderer = std::make_unique<Renderer>(pipeline, swapchain);
        }


    }

    void VulkanApplication::render() {
        const double &now = glfwGetTime();
        camera.update(window->getWindow(), deltaTime);

        lastFrame = now;
        ubo.view = camera.view();
        const auto &[width, height] = swapchain->getExtent();
        ubo.projection = camera.perspective(
            static_cast<float>(width) /
            static_cast<float>(height)
        );
        cameraBuffer.setData(std::bit_cast<std::byte*>(&ubo));

        renderer->render(meshes);
    }

    // void VulkanApplication::run() {
    //     const std::string &file = "../../src/editor/resources/models/sponza/Sponza.gltf";
    //     const std::string &path = std::filesystem::path(file).remove_filename().string();
    //
    //     std::vector<VulkanDescriptorPoolExpanding::PoolSizeRatio> ratios = {
    //         {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
    //         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
    //         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
    //         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
    //     };
    //     auto descriptorPool = std::make_shared<VulkanDescriptorPoolExpanding>(device, 1000, ratios);
    //
    //     auto camera = Camera(glm::vec3{0.0f, 0.0f, 0.0f});
    //     auto cameraBuffer = VulkanBuffer(
    //         device,
    //         BufferUsage::Uniform,
    //         1,
    //         sizeof(UniformBufferObject)
    //     );
    //
    //     UniformBufferObject ubo{
    //         camera.view(),
    //         camera.perspective(static_cast<float>(1920) / static_cast<float>(1080))
    //     };
    //
    //     Assimp::Importer importer;
    //     const auto &scene = importer.ReadFile(file, aiProcessPreset_TargetRealtime_Fast);
    //     if (!scene)
    //         throw std::runtime_error("Failed to load model from file");
    //
    //     std::vector<VulkanMesh> meshes{};
    //     meshes.reserve(scene->mNumMeshes);
    //
    //     for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
    //         const auto &aiMesh = scene->mMeshes[i];
    //         const auto &hasColors = aiMesh->HasVertexColors(0);
    //         const auto &hasUvs = aiMesh->HasTextureCoords(0);
    //
    //         std::vector<Vertex> vertices(aiMesh->mNumVertices);
    //         for (uint32_t j = 0; j < aiMesh->mNumVertices; j++) {
    //             const auto &vertex = aiMesh->mVertices[j];
    //             // TODO: Instead of storing default values for each vertex where a color or UV is missing, we should compact this down to save memory
    //             const auto &color = hasColors ? aiMesh->mColors[0][j] : aiColor4D{1.0F, 1.0F, 1.0F, 1.0F};
    //             const auto &textureCoord = hasUvs ? aiMesh->mTextureCoords[0][j] : aiVector3D{1.0F, 1.0F, 1.0F};
    //             const auto &normal = aiMesh->mNormals[j];
    //
    //             vertices[j] = Vertex{
    //                 .position = {vertex.x, vertex.y, vertex.z},
    //                 .color = {color.r, color.g, color.b, color.a},
    //                 .uv = {textureCoord.x, textureCoord.y},
    //                 .normal = {normal.x, normal.y, normal.z}
    //             };
    //         }
    //
    //         std::vector<uint32_t> indices(aiMesh->mNumFaces * 3);
    //         for (uint32_t j = 0; j < aiMesh->mNumFaces; j++) {
    //             const auto &face = aiMesh->mFaces[j];
    //             if (face.mNumIndices != 3) {
    //                 spdlog::warn("Skipping face with {} indices", face.mNumIndices);
    //                 continue;
    //             }
    //
    //             indices[j * 3] = face.mIndices[0];
    //             indices[j * 3 + 1] = face.mIndices[1];
    //             indices[j * 3 + 2] = face.mIndices[2];
    //         }
    //
    //         aiString imagePath;
    //         const auto &aiMaterial = scene->mMaterials[scene->mMeshes[i]->mMaterialIndex];
    //         if (aiMaterial == nullptr)
    //             throw std::runtime_error("Material is nullptr");
    //
    //         aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &imagePath);
    //         const auto &texture = scene->GetEmbeddedTexture(imagePath.C_Str());
    //
    //         std::shared_ptr<VulkanImage> image;
    //         if (texture == nullptr) {
    //             image = std::make_shared<VulkanImage>(
    //                 VulkanImage::from(
    //                     device,
    //                     path + imagePath.C_Str()
    //                 )
    //             );
    //         } else {
    //             image = std::make_shared<VulkanImage>(
    //                 VulkanImage::from(
    //                     device,
    //                     std::bit_cast<std::byte*>(texture->pcData),
    //                     texture->mWidth
    //                 )
    //             );
    //         }
    //
    //         auto descriptor = descriptorPool->allocate(*pbrOpaqueShader.getDescriptorSetLayout());
    //         descriptor->writeUniformBuffer(0, cameraBuffer, 0, cameraBuffer.getSize());
    //
    //         auto imageView = std::make_shared<VulkanImageView>(device, image, VK_IMAGE_ASPECT_COLOR_BIT);
    //         descriptor->writeCombinedImageSampler(1, *imageView);
    //
    //         auto material = std::make_shared<Material>(
    //             pipeline,
    //             image,
    //             imageView,
    //             descriptor,
    //             MaterialPass::Opaque
    //         );
    //
    //         meshes.emplace_back(device);
    //         meshes[i].setVertices(vertices);
    //         meshes[i].setIndices(indices, PrimitiveTopology::TriangleList);
    //         meshes[i].setMaterial(material);
    //     }
    //
    //     double old = glfwGetTime();
    //     double lastFrame = old;
    //     uint32_t fps = 0;
    //     while (!window->shouldClose()) {
    //         if (window->update()) {
    //             swapchain->invalidate();
    //             // TODO: Recreating the entire renderer is probably overkill, need a better way to recreate framebuffers on resize triggered from window
    //             renderer = std::make_unique<Renderer>(pipeline, swapchain);
    //         }
    //
    //         const double &now = glfwGetTime();
    //         double deltaTime = now - lastFrame;
    //         camera.update(window->getWindow(), deltaTime);
    //
    //         lastFrame = now;
    //         ubo.view = camera.view();
    //         const auto &[width, height] = swapchain->getExtent();
    //         ubo.projection = camera.perspective(
    //             static_cast<float>(width) /
    //             static_cast<float>(height)
    //         );
    //         cameraBuffer.setData(std::bit_cast<std::byte*>(&ubo));
    //
    //         renderer->render(meshes);
    //
    //         fps++;
    //         if (now - old >= 1) {
    //             spdlog::info("FPS: {}", fps);
    //             old = now;
    //             fps = 0;
    //         }
    //     }
    // }
}
