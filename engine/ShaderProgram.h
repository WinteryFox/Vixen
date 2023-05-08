#pragma once

#include <vector>
#include <memory>
#include "ShaderModule.h"

namespace Vixen::Engine {
    class ShaderProgram {
        std::vector<std::shared_ptr<ShaderModule>> modules;

    public:
        explicit ShaderProgram(const std::vector<std::shared_ptr<ShaderModule>> &modules) : modules(modules) {}
    };
}
