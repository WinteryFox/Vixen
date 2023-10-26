#pragma once

#include <memory>
#include "ShaderModule.h"

namespace Vixen {
    template<class T, class = std::enable_if<std::is_base_of<ShaderModule, T>::value>>
    class ShaderProgram {
    protected:
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
