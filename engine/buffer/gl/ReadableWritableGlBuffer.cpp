#include "ReadableWritableGlBuffer.h"

namespace Vixen::Engine {
    ReadableWritableGlBuffer::ReadableWritableGlBuffer(const size_t &size, BufferUsage bufferUsage,
                                                       AllocationUsage allocationUsage)
            : GlBuffer(size, bufferUsage, allocationUsage) {}
}
