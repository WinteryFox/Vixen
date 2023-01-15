#include "ShaderModule.h"

namespace Vixen::Engine::Gl {
    ShaderModule::ShaderModule(Stage stage, const std::string &source, const std::string &entry) : Engine::ShaderModule(stage, source, entry), module(0) {
        spirv_cross::CompilerGLSL glslCompiler(binary);
        spirv_cross::CompilerGLSL::Options glslOptions;

        glslOptions.version = 450;
        //glslOptions.vulkan_semantics = true;
        glslCompiler.set_common_options(glslOptions);
        auto crossed = glslCompiler.compile();
        spdlog::trace("Cross-compiled GLSL shader\n{}", crossed);

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
        auto src = crossed.c_str();
        glShaderSource(module, 1, &src, nullptr);
        glCompileShader(module);

        GLint status;
        glGetShaderiv(module, GL_COMPILE_STATUS, &status);
        if (!status) {
            GLint size;
            glGetShaderiv(module, GL_INFO_LOG_LENGTH, &size);
            char log[size];
            glGetShaderInfoLog(module, size, nullptr, log);
            spdlog::error("Failed to compile GL shader module: {}", std::string(log));
            throw std::runtime_error("Failed to compile GL shader module.");
        }
    }

    ShaderModule::~ShaderModule() {
        glDeleteShader(module);
    }
}
