#pragma once

#include <cstdint>

namespace Vixen {
    class Buffer {
    public:
        enum class Usage {
            Vertex = 1 << 0,
            Index = 1 << 1,
            Uniform = 1 << 2,
            CopySource = 1 << 3,
            CopyDestination = 1 << 4
        };

        Buffer(Usage usage, uint32_t size);

        ~Buffer();
    };
}
