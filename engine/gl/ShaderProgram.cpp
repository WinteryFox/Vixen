#include "ShaderProgram.h"

namespace Vixen::Engine::Gl {
    ShaderProgram::ShaderProgram(const std::vector<std::shared_ptr<Gl::ShaderModule>> &modules)
            : Engine::ShaderProgram<Gl::ShaderModule>(modules) {
        program = glCreateProgram();
        for (const auto &m: modules)
            glAttachShader(program, m->module);
        glLinkProgram(program);

        int success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            GLint size;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &size);
            char log[size];
            glGetProgramInfoLog(program, size, nullptr, log);
            spdlog::error("Failed to link GL shader program: {}", std::string(log));
            throw std::runtime_error("Failed to link GL shader program");
        }
    }

    ShaderProgram::~ShaderProgram() {
        glDeleteProgram(program);
    }

    void ShaderProgram::bind() const {
        glUseProgram(program);
    }

    void ShaderProgram::unbind() const {
        glUseProgram(0);
    }
}
