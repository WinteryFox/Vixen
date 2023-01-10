#include <spirv_cross/spirv_cross.hpp>
#include "ShaderModule.h"

namespace Vixen::Engine {
    ShaderModule::ShaderModule(ShaderModule::Stage stage, const std::string &source, std::string entry)
            : stage(stage), entry(std::move(entry)) {
        shaderc::Compiler compiler;
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
        resources = cross.get_shader_resources();
    }
}
