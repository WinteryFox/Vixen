#pragma once

#include <spirv_cross/spirv_cpp.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <GL/glew.h>
#include "../ShaderModule.h"

namespace Vixen::Engine {
    struct GlShaderModule : ShaderModule {
        unsigned int module;

        GlShaderModule(Stage stage, const std::string &source, const std::string &entry = "main");

        GlShaderModule(const GlShaderModule &) = delete;

        GlShaderModule &operator=(const GlShaderModule &) = delete;

        ~GlShaderModule();
    };
}
