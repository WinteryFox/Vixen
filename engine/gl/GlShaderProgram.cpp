#include "GlShaderProgram.h"

namespace Vixen::Vk {
    GlShaderProgram::GlShaderProgram(const std::vector<std::shared_ptr<GlShaderModule>> &modules)
            : Vk::ShaderProgram<GlShaderModule>(modules) {
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

    GlShaderProgram::~GlShaderProgram() {
        glDeleteProgram(program);
    }

    void GlShaderProgram::bind() const {
        glUseProgram(program);
    }

    void GlShaderProgram::unbind() {
        glUseProgram(0);
    }
}
