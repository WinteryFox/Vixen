#pragma once

#include <cstdint>

namespace Vixen {
    enum class BufferUsage : std::uint32_t {
        Vertex = 1 << 0,
        Index = 1 << 1,
        Uniform = 1 << 2,
        CopySource = 1 << 3,
        CopyDestination = 1 << 4
    };

    inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
        return static_cast<BufferUsage>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
    }

    inline bool operator&(BufferUsage a, BufferUsage b) {
        return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b);
    }
}
