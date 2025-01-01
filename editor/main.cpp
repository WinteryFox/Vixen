#include <fstream>

#include "core/Application.h"
#include "core/RenderingDevice.h"

int main() {
    try {
        auto application = Vixen::Application(
            Vixen::RenderingDriver::Vulkan,
            "Vixen Engine",
            {1, 0, 0}
        );

        const auto device = application.getDisplayServer()->getRenderingDevice();
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

        std::ifstream file("../../editor/resources/shaders/pbr.vertex.glsl");
        std::string source = (std::stringstream{} << file.rdbuf()).str();
        file.close();

        const auto binary = device->compileSpirvFromSource(Vixen::ShaderStage::Vertex, source,
                                                           Vixen::ShaderLanguage::GLSL);
        const auto shader = device->createShaderFromBytecode(binary);
        spdlog::error("Shader compiled successfully");
        device->destroyShader(shader);

        application.run();
    } catch (const std::runtime_error &e) {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
