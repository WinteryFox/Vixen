#pragma once

#include <cstdint>

#include "core/utility/Bitmask.h"

namespace Vixen {
    enum class BufferUsageBits : uint32_t {
        CopySource = 1u << 0,
        CopyDestination = 1u << 1,
        UniformTexel = 1u << 2,
        StorageTexel = 1u << 3,
        Uniform = 1u << 4,
        Storage = 1u << 5,
        Index = 1u << 6,
        Vertex = 1u << 7,
        Indirect = 1u << 8
    };

    template <>
    struct EnableFlags<BufferUsageBits> : std::true_type {};

    using BufferUsageFlags = Flags<BufferUsageBits>;
}
