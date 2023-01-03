#pragma once

#include <glm/vec3.hpp>

namespace Vixen::Engine {
    struct Context {
        glm::vec3 clearColor;

#ifdef DEBUG
        bool debug = true;
#else
        bool debug = false;
#endif
    };
}
