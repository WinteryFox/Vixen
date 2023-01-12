#include "ShaderModule.h"

namespace Vixen::Engine {
    ShaderModule::ShaderModule(ShaderModule::Stage stage, const std::string &source, std::string entry)
            : stage(stage), entry(std::move(entry)) {
        EShLanguage s = EShLanguage::EShLangCount;
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
        shader.setStrings(reinterpret_cast<const char *const *>(source.c_str()), 1);
        shader.setEnvInput(glslang::EShSourceGlsl, s, glslang::EShClientVulkan, 450);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
        shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);

        shader.setSourceEntryPoint(this->entry.c_str());

        EShMessages messages = EShMsgDefault;
        std::string preprocessed;
        glslang::TShader::ForbidIncluder includer;
        TBuiltInResource resources = GetDefaultResources();
        if (!shader.parse(&resources, 450, true, messages, includer)) {
        //if (!shader.preprocess(nullptr, 450, ENoProfile, false, false, messages, &preprocessed, includer)) {
            spdlog::error("Failed to pre-process shader");
            throw std::runtime_error("Failed to pre-process shader");
        }

        /*glslang::TIntermediate intermediate{};
        glslang::SpvOptions options;
#ifdef DEBUG
        options.generateDebugInfo = true;
        options.disableOptimizer = true;
        options.optimizeSize = false;
#else
        options.disableOptimizer = false;
        options.disableOptimizer = false;
        options.optimizeSize = true;
#endif
        options.validate = true;
        glslang::GlslangToSpv(intermediate, binary, &options);*/

        glslang::FinalizeProcess();

        /*shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        options.SetTargetSpirv(shaderc_spirv_version_1_6);
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
#ifdef DEBUG
        options.SetGenerateDebugInfo();
#else
        options.SetOptimizationLevel(shaderc_optimization_level_performance);
#endif

        auto preprocessResult = compiler.PreprocessGlsl(
                source,
                static_cast<shaderc_shader_kind>(stage),
                "",
                options
        );
        if (preprocessResult.GetCompilationStatus() != shaderc_compilation_status_success) {
            spdlog::error("Failed to preprocess shader: {}", preprocessResult.GetErrorMessage());
            throw std::runtime_error("Failed to preprocess shader");
        }
        std::string preprocessed{preprocessResult.begin(), preprocessResult.end()};
        spdlog::trace("Preprocessed shader with {} warnings\n{}", preprocessResult.GetNumWarnings(), preprocessed);

        auto compilerResult = compiler.CompileGlslToSpv(
                preprocessed,
                static_cast<shaderc_shader_kind>(stage),
                "",
                this->entry.c_str(),
                options
        );
        if (compilerResult.GetCompilationStatus() != shaderc_compilation_status_success) {
            spdlog::error("Failed to compile shader: {}", compilerResult.GetErrorMessage());
            throw std::runtime_error("Failed to compile shader");
        }
        binary = std::vector<uint32_t>{compilerResult.begin(), compilerResult.end()};
        spdlog::trace("Compiled shader to SPIR-V binary with {} warnings. {}", compilerResult.GetNumWarnings(),
                      spdlog::to_hex(binary.begin(), binary.end()));

        spirv_cross::Compiler cross{binary};
        resources = cross.get_shader_resources();*/
    }
}
