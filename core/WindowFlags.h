#pragma once

#include "Bitmask.h"

namespace Vixen {
    enum class WindowFlags : int64_t {
        Resizable = 1 << 0,
        Borderless = 1 << 1,
        Transparent = 1 << 2,
        AlwaysOnTop = 1 << 3
    };

    DECLARE_BITMASK(WindowFlags);
}
