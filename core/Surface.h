#pragma once

#include <glm/vec2.hpp>

#include "VSyncMode.h"
#include "WindowMode.h"

namespace Vixen {
    struct Surface {
        glm::uvec2 resolution;
        bool isResizeRequired;
        WindowMode windowMode;
        VSyncMode vsyncMode;

        virtual ~Surface() = default;
    };
}
