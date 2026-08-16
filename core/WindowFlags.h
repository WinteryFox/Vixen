#pragma once

#include <cstdint>

#include "Bitmask.h"

namespace Vixen {
    enum class WindowBits : uint32_t {
        Resizable = 1u << 0,
        Borderless = 1u << 1,
        Transparent = 1u << 2,
        AlwaysOnTop = 1u << 3
    };

    template <>
    struct EnableFlags<WindowBits> : std::true_type {};

    using WindowFlags = Flags<WindowBits>;
}
