#pragma once

#include <vector>
#include <memory>
#include "ShaderModule.h"

namespace Vixen {
    template<class T, class = std::enable_if<std::is_base_of<ShaderModule, T>::value>>
    class ShaderProgram {
    protected:
        std::vector<std::shared_ptr<T>> modules;

    public:
        explicit ShaderProgram(const std::vector<std::shared_ptr<T>> &modules) : modules(modules) {}

        const std::vector<std::shared_ptr<T>> &getModules() const {
            return modules;
        }
    };
}
