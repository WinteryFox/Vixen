#pragma once

#include <cstdint>

namespace Vixen {
    struct BufferCopyRegion {
        uint64_t sourceOffset;
        uint64_t destinationOffset;
        uint64_t size;
    };
}
