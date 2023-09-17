#include "VkShaderModule.h"

namespace Vixen::Engine {
    VkShaderModule::VkShaderModule(const std::shared_ptr<Device> &device, ShaderModule::Stage stage, const std::string &source, const std::string &entrypoint)
            : ShaderModule(stage, entrypoint), device(device) {
        EShLanguage s;
        switch (stage) {
            case Stage::VERTEX:
                s = EShLanguage::EShLangVertex;
                break;
            case Stage::FRAGMENT:
                s = EShLanguage::EShLangFragment;
                break;
            default:
                spdlog::error("Unsupported stage for shader module");
                throw std::runtime_error("Unsupported stage for shader module");
        }

        glslang::InitializeProcess();
        glslang::TShader shader{s};
        auto src = source.c_str();
        shader.setStrings(&src, 1);
        shader.setEnvInput(glslang::EShSourceGlsl, s, glslang::EShClientVulkan, 130);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
        shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);
#ifdef DEBUG
        shader.setDebugInfo(true);
#endif

        shader.setEntryPoint(entrypoint.c_str());
        shader.setSourceEntryPoint(entrypoint.c_str());

        EShMessages messages = EShMsgDefault;
        glslang::TShader::ForbidIncluder includer;
        auto resources = GetDefaultResources();
        if (!shader.parse(resources, 100, true, messages, includer)) {
            auto log = shader.getInfoLog();
            spdlog::error("Failed to pre-process shader; {}", log);
            throw std::runtime_error("Failed to pre-process shader");
        }

        auto intermediate = shader.getIntermediate();
        glslang::SpvOptions options;
#ifdef DEBUG
        options.generateDebugInfo = true;
        options.disableOptimizer = true;
        options.optimizeSize = false;
        options.stripDebugInfo = false;
#else
        options.disableOptimizer = false;
        options.disableOptimizer = false;
        options.optimizeSize = true;
        options.stripDebugInfo = true;
#endif
        options.validate = true;
        spv::SpvBuildLogger logger;
        std::vector<uint32_t> binary{};
        glslang::GlslangToSpv(*intermediate, binary, &logger, &options);

#ifdef DEBUG
        std::stringstream stream;
        spv::Disassemble(stream, binary);
        spdlog::debug("Compile shader module to SPIR-V\n{}", stream.str());
#endif

        glslang::FinalizeProcess();
        if (!logger.getAllMessages().empty())
            spdlog::debug("{}", logger.getAllMessages());
        spdlog::trace("Compiled shader to SPIR-V binary {}", spdlog::to_hex(binary.begin(), binary.end()));

        VkShaderModuleCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = binary.size(),
            .pCode = binary.data()
        };
        checkVulkanResult(vkCreateShaderModule(device->getDevice(), &info, nullptr, &module),
                          "Failed to create shader module");
    }

    VkShaderModule::~VkShaderModule() {
        vkDestroyShaderModule(device->getDevice(), module, nullptr);
    }
}
