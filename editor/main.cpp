#include <fstream>

#include "core/Application.h"
#include "core/RenderingDevice.h"
#include "core/command/CommandBufferType.h"

int main() {
    try {
        auto application = Vixen::Application(
            Vixen::RenderingDriver::Vulkan,
            "Vixen " ENGINE_VERSION,
            {ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH}
        );

        const auto device = application.getDisplayServer()->getRenderingDevice();
        const auto context = application.getDisplayServer()->getRenderingContext();
        const auto buffer = device->createBuffer(
            Vixen::BufferUsage::Vertex, 1, sizeof(float) * 3);
        const auto image = device->createImage(
            {
                .format = Vixen::R8G8B8A8_SRGB,
                .width = 1,
                .height = 1,
                .depth = 1,
                .layerCount = 1,
                .mipmapCount = 1,
                .type = Vixen::ImageType::TwoD,
                .samples = Vixen::ImageSamples::One,
                .usage = Vixen::ImageUsage::ColorAttachment
            },
            {
                .format = Vixen::R8G8B8A8_SRGB,
                .swizzleRed = Vixen::ImageSwizzle::Red,
                .swizzleGreen = Vixen::ImageSwizzle::Green,
                .swizzleBlue = Vixen::ImageSwizzle::Blue,
                .swizzleAlpha = Vixen::ImageSwizzle::Alpha
            }
        );
        spdlog::error(buffer->getCount());
        spdlog::error(image->format.width);
        device->destroyBuffer(buffer);
        device->destroyImage(image);

        std::ifstream vertexFile("../../editor/resources/shaders/pbr.vertex.glsl");
        std::string vertexSource = (std::stringstream{} << vertexFile.rdbuf()).str();
        vertexFile.close();
        const auto vertexSpirv = device->compileSpirvFromSource(Vixen::ShaderStage::Vertex, vertexSource,
                                                                Vixen::ShaderLanguage::GLSL);

        std::ifstream fragmentFile("../../editor/resources/shaders/pbr.fragment.glsl");
        std::string fragmentSource = (std::stringstream{} << fragmentFile.rdbuf()).str();
        fragmentFile.close();
        const auto fragmentSpirv = device->compileSpirvFromSource(Vixen::ShaderStage::Fragment, fragmentSource,
                                                                  Vixen::ShaderLanguage::GLSL);

        const auto shader = device->createShaderFromSpirv(
            "PBR",
            {
                {
                    .stage = Vixen::ShaderStage::Vertex,
                    .spirv = vertexSpirv
                },
                {
                    .stage = Vixen::ShaderStage::Fragment,
                    .spirv = fragmentSpirv
                }
            }
        );
        spdlog::error("Shader compiled successfully");
        device->destroyShader(shader);
        const auto commandPool = device->createCommandPool(0, Vixen::CommandBufferType::Primary);
        const auto commandBuffer = device->createCommandBuffer(commandPool);
        device->resetCommandPool(commandPool);
        device->destroyCommandPool(commandPool);

        const auto surface = context->createSurface(application.getDisplayServer()->getMainWindow());

        const auto swapchain = device->createSwapchain(surface);
        device->resizeSwapchain(nullptr, swapchain, 1);
        device->destroySwapchain(swapchain);

        context->destroySurface(surface);

        application.run();
    } catch (const std::runtime_error &e) {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
