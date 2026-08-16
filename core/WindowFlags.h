#pragma once

#include "Bitmask.h"

namespace Vixen {
    enum class WindowBits : uint32_t {
        Resizable = 1 << 0,
        Borderless = 1 << 1,
        Transparent = 1 << 2,
        AlwaysOnTop = 1 << 3
    };

    template <>
    struct EnableFlags<WindowBits> : std::true_type {};

    using WindowFlags = Flags<WindowBits>;
}
