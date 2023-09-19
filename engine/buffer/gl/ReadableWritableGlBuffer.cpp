#include "ReadableWritableGlBuffer.h"

namespace Vixen::Vk {
    ReadableWritableGlBuffer::ReadableWritableGlBuffer(const size_t &size, BufferUsage bufferUsage,
                                                       AllocationUsage allocationUsage)
            : GlBuffer(size, bufferUsage, allocationUsage) {}
}
