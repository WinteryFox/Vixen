#pragma once

#include <spirv_cpp.hpp>
#include <spirv_glsl.hpp>
#include <spdlog/spdlog.h>
#include <GL/glew.h>
#include <string>
#include "../ShaderModule.h"

namespace Vixen::Engine::Gl {
    struct ShaderModule : Vixen::Engine::ShaderModule {
        unsigned int module;

        ShaderModule(Stage stage, const std::string &source, const std::string &entry = "main");

        ShaderModule(const ShaderModule &) = delete;

        ShaderModule &operator=(const ShaderModule &) = delete;

        ~ShaderModule();
    };
}
