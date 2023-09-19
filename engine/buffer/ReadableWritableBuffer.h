#pragma once

#include "ReadableBuffer.h"

namespace Vixen::Vk {
    class ReadableWritableBuffer : public ReadableBuffer, public WritableBuffer {
    public:
        ReadableWritableBuffer(const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage);
    };
}
