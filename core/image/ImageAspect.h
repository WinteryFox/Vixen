#pragma once

#include "core/Bitmask.h"

namespace Vixen {
    enum class ImageAspect : uint32_t {
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 2
    };

    DECLARE_BITMASK(ImageAspect);
}
