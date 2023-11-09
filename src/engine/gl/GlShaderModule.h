#pragma once

#include <GL/glew.h>
#include <spdlog/spdlog.h>
#include "../ShaderModule.h"

namespace Vixen::Gl {
    class GlShaderModule : public ShaderModule {
        friend class GlShaderProgram;

    protected:
        unsigned int module;

    public:
        GlShaderModule(Stage stage, const std::string &source, const std::string &entry = "main");

        GlShaderModule(const GlShaderModule &) = delete;

        GlShaderModule &operator=(const GlShaderModule &) = delete;

        ~GlShaderModule();
    };
}
