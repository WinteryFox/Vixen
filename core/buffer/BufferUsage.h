#pragma once

#include <cstdint>

#include "core/Bitmask.h"

namespace Vixen {
    enum class BufferUsage : uint32_t {
        CopySource = 1 << 0,
        CopyDestination = 1 << 1,
        Texel = 1 << 2,
        Uniform = 1 << 3,
        Storage = 1 << 4,
        Vertex = 1 << 5,
        Index = 1 << 6,
        Indirect = 1 << 7
    };

    DECLARE_BITMASK(BufferUsage);
}
