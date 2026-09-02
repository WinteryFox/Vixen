#pragma once

#include <cstdint>
#include <type_traits>

#include "utility/Bitmask.h"

namespace Vixen {
    enum class DynamicStateBits : uint32_t {
        Viewport = 1u << 0,
        Scissor = 1u << 1,
        BlendConstants = 1u << 2
    };

    template <>
    struct EnableFlags<DynamicStateBits> : std::true_type {};

    using DynamicStateFlags = Flags<DynamicStateBits>;
}
