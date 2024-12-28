#pragma once

#include "core/Bitmask.h"

namespace Vixen {
    enum class ImageAspect : int64_t {
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 2
    };

    DECLARE_BITMASK(ImageAspect);
}
