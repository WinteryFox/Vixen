#include "ShaderModule.h"

namespace Vixen::Engine::Gl {
    ShaderModule::ShaderModule(Stage stage, const std::string &source) : Vixen::Engine::ShaderModule(stage), module(0) {
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
        auto src = source.c_str();
        glShaderSource(module, 1, &src, nullptr);
        glCompileShader(module);

        int success;
        char log[512];
        glGetShaderiv(module, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(module, 512, nullptr, log);
            spdlog::error("Failed to compile shader: {}", log);
        }
    }

    ShaderModule::~ShaderModule() {
        glDeleteShader(module);
    }
}
