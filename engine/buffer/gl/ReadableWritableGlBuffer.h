#pragma once

#include "../ReadableWritableBuffer.h"
#include "ReadableGlBuffer.h"
#include "WritableGlBuffer.h"

namespace Vixen::Engine {
    class ReadableWritableGlBuffer : private virtual GlBuffer, public virtual ReadableWritableBuffer {
    public:
        ReadableWritableGlBuffer(const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage);
    };
}
