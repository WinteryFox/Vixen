#include <fstream>

#include "core/Application.h"
#include "core/RenderingDevice.h"
#include "core/RenderingDeviceDriver.h"
#include "core/command/CommandBufferType.h"

int main() {
    try {
        auto application = Vixen::Application(
            Vixen::RenderingDriver::Vulkan,
            "Vixen " ENGINE_VERSION,
            {ENGINE_VERSION_MAJOR, ENGINE_VERSION_MINOR, ENGINE_VERSION_PATCH}
        );

        const auto device = application.getRenderingDevice()->getRenderingDeviceDriver();

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
        device->destroyShader(shader);

        application.run();
    } catch (const std::runtime_error &e) {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
