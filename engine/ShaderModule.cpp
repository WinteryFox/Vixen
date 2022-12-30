#include "ShaderModule.h"

namespace Vixen::Engine {
    ShaderModule::ShaderModule(ShaderModule::Stage stage, const std::string &source, const std::string &entry)
            : stage(stage), entry(entry), binary() {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        options.SetForcedVersionProfile(460, shaderc_profile_none);
        options.SetSourceLanguage(shaderc_source_language_glsl);
        options.SetTargetSpirv(shaderc_spirv_version_1_6);
#ifdef DEBUG
        options.SetGenerateDebugInfo();
#endif
        //options.SetOptimizationLevel(shaderc_optimization_level_performance);

        auto preprocessResult = compiler.PreprocessGlsl(
                source,
                static_cast<shaderc_shader_kind>(stage),
                source.c_str(),
                options
        );
        if (preprocessResult.GetCompilationStatus() != shaderc_compilation_status_success) {
            spdlog::error("Failed to preprocess shader: {}", preprocessResult.GetErrorMessage());
            throw std::runtime_error("Failed to preprocess shader");
        }
        std::string preprocessed{preprocessResult.cbegin(), preprocessResult.cend()};
        spdlog::trace("Preprocessed shader with {} warnings\n{}", preprocessResult.GetNumWarnings(), preprocessed);

        auto compilerResult = compiler.CompileGlslToSpv(
                preprocessed,
                static_cast<shaderc_shader_kind>(stage),
                preprocessed.c_str(),
                entry.c_str(),
                options
        );
        if (compilerResult.GetCompilationStatus() != shaderc_compilation_status_success) {
            spdlog::error("Failed to compile shader: {}", compilerResult.GetErrorMessage());
            throw std::runtime_error("Failed to compile shader");
        }
        binary = std::vector<char>(compilerResult.cbegin(), compilerResult.cend());
        spdlog::trace("Compiled shader to SPIR-V binary with {} warnings {}", compilerResult.GetNumWarnings(),
                      spdlog::to_hex(binary));
    }
}
