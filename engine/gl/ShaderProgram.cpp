#include "ShaderProgram.h"

namespace Vixen::Engine::Gl {
    ShaderProgram::ShaderProgram(const std::vector<std::shared_ptr<Gl::ShaderModule>> &modules)
            : Engine::ShaderProgram<Gl::ShaderModule>(modules) {
        program = glCreateProgram();
        for (const auto &m: modules)
            glAttachShader(program, m->module);
        glLinkProgram(program);

        int success;
        char log[512];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 512, nullptr, log);
            spdlog::error("Failed to link shader program: {}", log);
        }
    }

    ShaderProgram::~ShaderProgram() {
        glDeleteProgram(program);
    }

    void ShaderProgram::bind() const {
        glUseProgram(program);
    }
}
