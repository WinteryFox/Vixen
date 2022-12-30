#include "ShaderModule.h"

namespace Vixen::Engine::Gl {
    ShaderModule::ShaderModule(Stage stage, const std::string &source, const std::string &entry) : Engine::ShaderModule(stage, source, entry), module(0) {
        spirv_cross::CompilerGLSL glslCompiler(spirvBinary);
        spirv_cross::CompilerGLSL::Options glslOptions;

        glslOptions.version = 450;
        glslCompiler.set_common_options(glslOptions);
        auto crossed = glslCompiler.compile();
        spdlog::trace("Cross-compiled GLSL shader\n{}", crossed);

        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
#ifdef DEBUG
        options.SetGenerateDebugInfo();
#else
        options.SetOptimizationLevel(shaderc_optimization_level_performance);
#endif
        auto result = compiler.CompileGlslToSpv(
                crossed,
                static_cast<shaderc_shader_kind>(stage),
                "",
                this->entry.c_str(),
                options
        );
        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            spdlog::error("Failed to compile GL shader from crossed source with {} warnings. {}", result.GetNumWarnings(), result.GetErrorMessage());
            throw std::runtime_error("Failed to compile GL shader from crossed source");
        }
        glslBinary = std::vector<uint32_t>(result.begin(), result.end());

        // TODO: spirv_cross::ShaderResources resources = glslCompiler.get_shader_resources();

        switch (stage) {
            case Stage::VERTEX:
                module = glCreateShader(GL_VERTEX_SHADER);
                break;
            case Stage::FRAGMENT:
                module = glCreateShader(GL_FRAGMENT_SHADER);
                break;
            default:
                spdlog::error("Unsupported shader stage");
                throw std::runtime_error("Unsupported shader stage");
        }
        glShaderBinary(1, &module, GL_SHADER_BINARY_FORMAT_SPIR_V, glslBinary.data(), glslBinary.size() * sizeof(uint32_t));
        glSpecializeShader(module, entry.c_str(), 0, nullptr, nullptr);
    }

    ShaderModule::~ShaderModule() {
        glDeleteShader(module);
    }
}
