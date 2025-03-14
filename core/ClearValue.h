#pragma once

#include <glm/vec4.hpp>

namespace Vixen {
    struct ClearValue {
        glm::vec4 color;
        float depth;
        uint32_t stencil;
    };
}
