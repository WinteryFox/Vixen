#pragma once

#include <glm/glm.hpp>

#include "WindowMode.h"
#include "VSyncMode.h"

namespace Vixen {
    class Surface {
    public:
        glm::ivec2 resolution;
        bool hasFramebufferSizeChanged;
        WindowMode windowMode;
        VSyncMode vsyncMode;

        virtual ~Surface() = default;
    };
}
