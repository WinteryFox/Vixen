#pragma once

#include <cstdint>

#include "Bitmask.h"

namespace Vixen {
    enum class QueueFamilyBits : uint32_t {
        Graphics = 1 << 0,
        Transfer = 1 << 1,
        Compute = 1 << 2
    };

    template <>
    struct EnableFlags<QueueFamilyBits> : std::true_type {};

    using QueueFamilyFlags = Flags<QueueFamilyBits>;
}
