#pragma once

#include <cstdint>

#include "core/Bitmask.h"

namespace Vixen {
    enum class BufferUsageBits : uint32_t {
        CopySource = 1u << 0,
        CopyDestination = 1u << 1,
        Texel = 1u << 2,
        Uniform = 1u << 3,
        Storage = 1u << 4,
        Vertex = 1u << 5,
        Index = 1u << 6,
        Indirect = 1u << 7
    };

    template <>
    struct EnableFlags<BufferUsageBits> : std::true_type {};

    using BufferUsageFlags = Flags<BufferUsageBits>;
} // namespace Vixen
