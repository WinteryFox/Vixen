#pragma once

#include <vector>
#include <memory>
#include "ShaderModule.h"

namespace Vixen::Engine {
    template<class T = ShaderModule>
    class ShaderProgram {
        std::vector<std::shared_ptr<T>> modules;

    public:
        explicit ShaderProgram(const std::vector<std::shared_ptr<T>> &modules) : modules(modules) {}
    };
}
