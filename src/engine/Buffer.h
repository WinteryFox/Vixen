#pragma once

#include <cstdint>

namespace Vixen {
    enum class BufferUsage : std::uint32_t {
        VERTEX = 1 << 0,
        INDEX = 1 << 1,
        UNIFORM = 1 << 2,
        COPY_SOURCE = 1 << 3,
        COPY_DESTINATION = 1 << 4
    };

    inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
        return static_cast<BufferUsage>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
    }

    inline bool operator&(BufferUsage a, BufferUsage b) {
        return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b);
    }
}
