#pragma once

#include "../ReadableBuffer.h"
#include "GlBuffer.h"

namespace Vixen::Vk {
    class ReadableGlBuffer : private virtual GlBuffer, public virtual ReadableBuffer {
    public:
        ReadableGlBuffer(const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage);
    };
}
