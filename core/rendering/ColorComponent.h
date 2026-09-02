#pragma once

#include <cstdint>
#include <type_traits>

#include "utility/Bitmask.h"

namespace Vixen {
    enum class ColorComponentBits : uint32_t {
        Red = 1u << 0,
        Green = 1u << 1,
        Blue = 1u << 2,
        Alpha = 1u << 3
    };

    template <>
    struct EnableFlags<ColorComponentBits> : std::true_type {};

    using ColorComponentFlags = Flags<ColorComponentBits>;
}
