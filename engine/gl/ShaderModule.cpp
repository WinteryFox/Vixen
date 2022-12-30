#include "ShaderModule.h"

namespace Vixen::Engine::Gl {
    ShaderModule::ShaderModule(Stage stage, const std::string &source, const std::string &entry) : Vixen::Engine::ShaderModule(stage, source, entry), module(0) {
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
        glShaderBinary(1, &module, GL_SHADER_BINARY_FORMAT_SPIR_V, binary.data(), binary.size());
        glSpecializeShader(module, entry.c_str(), 0, nullptr, nullptr);

        GLint status;
        glGetShaderiv(module, GL_COMPILE_STATUS, &status);
        if (GL_FALSE == status) {
            char log[512];
            glGetShaderInfoLog(module, 512, nullptr, log);
            spdlog::error("Failed to compile shader module. {}", log);
            throw std::runtime_error("Failed to compile shader module");
        }
    }

    ShaderModule::~ShaderModule() {
        glDeleteShader(module);
    }
}
