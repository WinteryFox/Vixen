#pragma once

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <string>
#include <utility>

#ifdef DEBUG

#include <glslang/SPIRV/disassemble.h>

#endif

namespace Vixen::Engine {
    class ShaderModule {
    public:
        enum class Stage {
            VERTEX,
            FRAGMENT,
        };

        Stage stage;

        std::string entry;

        std::vector<uint32_t> binary{};

        ShaderModule(Stage stage, const std::string &source, std::string entry);
    };
}
