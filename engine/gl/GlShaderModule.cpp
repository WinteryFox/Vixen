#include "GlShaderModule.h"

namespace Vixen::Gl {
    GlShaderModule::GlShaderModule(Stage stage, const std::string &source, const std::string &entry)
                : ShaderModule(stage, entry, {}, {}), module(0) {
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

    GlShaderModule::~GlShaderModule() {
        glDeleteShader(module);
    }
}
