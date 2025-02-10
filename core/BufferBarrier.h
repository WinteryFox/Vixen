#pragma once

#include "BarrierAccessFlags.h"
#include "buffer/Buffer.h"

namespace Vixen {
    struct BufferBarrier {
        Buffer *buffer;
        BarrierAccessFlags sourceAccess;
        BarrierAccessFlags destinationAccess;
        uint64_t offset;
        uint64_t size;
    };
}
