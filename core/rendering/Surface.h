#pragma once

#include <glm/vec2.hpp>

#include "core/display/VSyncMode.h"
#include "core/display/WindowMode.h"

namespace Vixen {
    struct Surface {
        glm::uvec2 resolution{0, 0};
        bool isResizeRequired = false;
        WindowMode windowMode = WindowMode::Windowed;
        VSyncMode vsyncMode = VSyncMode::Enabled;

        virtual ~Surface() = default;
    };
}
