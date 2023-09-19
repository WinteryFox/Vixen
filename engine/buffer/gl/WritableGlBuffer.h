#pragma once

#include "../WritableBuffer.h"
#include "GlBuffer.h"

namespace Vixen::Vk {
    class WritableGlBuffer : public virtual GlBuffer, public virtual WritableBuffer {
    public:
        using WritableBuffer::write;

        WritableGlBuffer(const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage);

        WritableGlBuffer &write(const void *data, std::size_t dataSize, std::size_t offset) override;
    };
}
