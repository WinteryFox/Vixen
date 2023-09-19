#pragma once

#include <volk.h>
#include <glslang/Public/ShaderLang.h>
#include <spdlog/spdlog.h>
#include <glslang/SPIRV/SpvTools.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/SPIRV/disassemble.h>
#include <glslang/Public/ResourceLimits.h>
#include <spdlog/fmt/bin_to_hex.h>
#include "../ShaderProgram.h"

namespace Vixen::Engine {
    class VkShaderProgram : public ShaderProgram {
    public:
        explicit VkShaderProgram(const std::vector<std::shared_ptr<ShaderModule>> &modules);
    };
}
