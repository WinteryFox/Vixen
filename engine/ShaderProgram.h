#pragma once

#include <memory>
#include "ShaderModule.h"

namespace Vixen {
    template<class T> requires std::is_base_of_v<ShaderModule, T>
    class ShaderProgram {
        std::shared_ptr<T> vertex;

        std::shared_ptr<T> fragment;

    public:
        ShaderProgram(const std::shared_ptr<T> &vertex, const std::shared_ptr<T> &fragment)
                : vertex(vertex), fragment(fragment) {}

        const std::shared_ptr<T> &getVertex() const {
            return vertex;
        }

        const std::shared_ptr<T> &getFragment() const {
            return fragment;
        }
    };
}
