#pragma once

#include <shaderc/shaderc.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <string>
#include <utility>

namespace Vixen::Engine {
    class ShaderModule {
    public:
        enum class Stage {
            VERTEX = shaderc_vertex_shader,
            FRAGMENT = shaderc_fragment_shader,
        };

        Stage stage;

        std::string entry;

        std::vector<uint32_t> spirvBinary{};

        std::vector<uint32_t> glslBinary{};

        ShaderModule(Stage stage, const std::string &source, std::string entry);
    };
}
