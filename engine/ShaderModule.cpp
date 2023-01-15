#include "ShaderModule.h"

namespace Vixen::Engine {
    ShaderModule::ShaderModule(ShaderModule::Stage stage, const std::string &source, std::string entry)
            : stage(stage), entry(std::move(entry)) {
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
        shader.setEnvInput(glslang::EShSourceGlsl, s, glslang::EShClientVulkan, 100);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
        shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);
        shader.setEnvInputVulkanRulesRelaxed();

        shader.setEntryPoint(this->entry.c_str());
        shader.setSourceEntryPoint(this->entry.c_str());

        EShMessages messages = EShMsgDefault;
        std::string preprocessed;
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
        glslang::GlslangToSpv(*intermediate, binary, &logger, &options);

#ifdef DEBUG
        std::stringstream stream;
        spv::Disassemble(stream, binary);
        spdlog::debug("Disassembled SPIR-V:\n{}", stream.str());
#endif

        glslang::FinalizeProcess();
        if (!logger.getAllMessages().empty())
            spdlog::debug("{}", logger.getAllMessages());
        spdlog::trace("Compiled shader to SPIR-V binary {}", spdlog::to_hex(binary.begin(), binary.end()));
    }
}
