#ifdef _WIN32

#include <windows.h>

#endif

#include <cstdlib>
#include <filesystem>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include "VkBuffer.h"
#include "VkDescriptorPoolExpanding.h"
#include "VkDescriptorSet.h"
#include "VkPipeline.h"
#include "VkRenderer.h"
#include "VkVixen.h"
#include "../../Camera.h"
#include "../../PrimitiveTopology.h"
#include "material/Material.h"

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

    const auto vertexShader = Vixen::Vk::VkShaderModule::Builder(Vixen::Vk::VkShaderModule::Stage::Vertex)
                              .addBinding({
                                  .binding = 0,
                                  .stride = sizeof(Vixen::Vk::Vertex),
                                  .rate = Vixen::Vk::VkShaderModule::Rate::Vertex
                              })
                              .addInput({
                                  .binding = 0,
                                  .location = 0,
                                  .size = sizeof(glm::vec3),
                                  .offset = offsetof(Vixen::Vk::Vertex, position)
                              })
                              .addInput({
                                  .binding = 0,
                                  .location = 1,
                                  .size = sizeof(glm::vec4),
                                  .offset = offsetof(Vixen::Vk::Vertex, color)
                              })
                              .addInput({
                                  .binding = 0,
                                  .location = 2,
                                  .size = sizeof(glm::vec2),
                                  .offset = offsetof(Vixen::Vk::Vertex, uv)
                              })
                              .compileFromFile(vixen.getDevice(), "../../src/editor/shaders/triangle.vert");
    const auto fragment = Vixen::Vk::VkShaderModule::Builder(Vixen::Vk::VkShaderModule::Stage::Fragment)
        .compileFromFile(vixen.getDevice(), "../../src/editor/shaders/triangle.frag");
    const auto program = Vixen::Vk::VkShaderProgram(vertexShader, fragment);

    int width;
    int height;
    vixen.getWindow()->getFramebufferSize(width, height);

    auto pipeline = Vixen::Vk::VkPipeline::Builder()
                    .setWidth(width)
                    .setHeight(height)
                    .setColorFormat(vixen.getSwapchain()->getColorFormat().format)
                    .setDepthFormat(vixen.getSwapchain()->getDepthFormat())
                    .build(vixen.getDevice(), program);

    auto renderer = std::make_unique<Vixen::Vk::VkRenderer>(pipeline, vixen.getSwapchain());

    const std::string& file = "../../src/engine/vk/test/models/sponza/Sponza.gltf";
    const std::string& path = std::filesystem::path(file).remove_filename().string();

    std::vector<Vixen::Vk::VkDescriptorPoolExpanding::PoolSizeRatio> ratios = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
    };
    auto descriptorPool = std::make_shared<Vixen::Vk::VkDescriptorPoolExpanding>(vixen.getDevice(), 1000, ratios);

    auto sampler = Vixen::Vk::VkSampler(vixen.getDevice());

    auto camera = Vixen::Camera(glm::vec3{0.0f, 0.0f, 0.0f});

    auto uniformBuffer = Vixen::Vk::VkBuffer(
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

    std::vector<Vixen::Vk::VkMesh> meshes{};
    meshes.reserve(scene->mNumMeshes);

    for (auto i = 0; i < scene->mNumMeshes; i++) {
        const auto& aiMesh = scene->mMeshes[i];
        const auto& hasColors = aiMesh->HasVertexColors(0);
        const auto& hasUvs = aiMesh->HasTextureCoords(0);

        std::vector<Vixen::Vk::Vertex> vertices(aiMesh->mNumVertices);
        for (uint32_t j = 0; j < aiMesh->mNumVertices; j++) {
            const auto& vertex = aiMesh->mVertices[j];
            // TODO: Instead of storing default values for each vertex where a color or UV is missing, we should compact this down to save memory
            const auto& color = hasColors ? aiMesh->mColors[0][j] : aiColor4D{1.0F, 1.0F, 1.0F, 1.0F};
            const auto& textureCoord = hasUvs ? aiMesh->mTextureCoords[0][j] : aiVector3D{1.0F, 1.0F, 1.0F};

            vertices[j] = Vixen::Vk::Vertex{
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

        std::shared_ptr<Vixen::Vk::VkImage> image;
        if (texture == nullptr) {
            image = std::make_shared<Vixen::Vk::VkImage>(
                Vixen::Vk::VkImage::from(
                    vixen.getDevice(),
                    path + imagePath.C_Str()
                )
            );
        } else {
            image = std::make_shared<Vixen::Vk::VkImage>(
                Vixen::Vk::VkImage::from(
                    vixen.getDevice(),
                    texture->achFormatHint,
                    reinterpret_cast<std::byte*>(texture->pcData),
                    texture->mWidth
                )
            );
        }

        auto descriptor = descriptorPool->allocate(*program.getDescriptorSetLayout());
        descriptor->writeUniformBuffer(0, uniformBuffer, 0, uniformBuffer.getSize());

        auto imageView = std::make_shared<Vixen::Vk::VkImageView>(image, VK_IMAGE_ASPECT_COLOR_BIT);
        descriptor->writeCombinedImageSampler(1, sampler, *imageView);

        auto material = std::make_shared<Vixen::Vk::Material>(
            pipeline,
            image,
            imageView,
            descriptor,
            Vixen::Vk::MaterialPass::Opaque
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
            renderer = std::make_unique<Vixen::Vk::VkRenderer>(pipeline, vixen.getSwapchain());
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
