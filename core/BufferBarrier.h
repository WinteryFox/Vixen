#pragma once

#include <cstdint>

#include "BarrierAccessFlags.h"

namespace Vixen {
    class Buffer;

    struct BufferBarrier {
        Buffer* buffer;
        BarrierAccessFlags sourceAccess;
        BarrierAccessFlags destinationAccess;
        uint64_t offset;
        uint64_t size;
    };
}
