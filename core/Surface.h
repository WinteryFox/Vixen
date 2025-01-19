#pragma once

#include <glm/glm.hpp>

#include "WindowMode.h"
#include "VSyncMode.h"

namespace Vixen {
    struct Surface {
        glm::ivec2 resolution;
        bool hasFramebufferSizeChanged;
        WindowMode windowMode;
        VSyncMode vsyncMode;
    };
}
