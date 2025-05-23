#pragma once

#include <glm/glm.hpp>

#include "WindowMode.h"
#include "VSyncMode.h"

namespace Vixen {
    struct Surface {
        glm::uvec2 resolution;
        bool isResizeRequired;
        WindowMode windowMode;
        VSyncMode vsyncMode;

        virtual ~Surface() = default;
    };
}
