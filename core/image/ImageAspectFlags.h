#pragma once

#include <cstdint>

#include "core/Bitmask.h"

namespace Vixen {
    enum class ImageAspectBits : uint32_t {
        Color = 1u << 0,
        Depth = 1u << 1,
        Stencil = 1u << 2
    };

    template <>
    struct EnableFlags<ImageAspectBits> : std::true_type {};

    using ImageAspectFlags = Flags<ImageAspectBits>;
}
