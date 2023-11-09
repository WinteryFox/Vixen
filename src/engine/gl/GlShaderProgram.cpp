#include "GlShaderProgram.h"

namespace Vixen::Gl {
    GlShaderProgram::GlShaderProgram(const std::shared_ptr<GlShaderModule> &vertex,
                                     const std::shared_ptr<GlShaderModule> &fragment)
            : ShaderProgram<GlShaderModule>(vertex, fragment) {
        program = glCreateProgram();

        if (vertex != nullptr)
            glAttachShader(program, vertex->module);

        if (fragment != nullptr)
            glAttachShader(program, fragment->module);

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
