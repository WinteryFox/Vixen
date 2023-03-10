#include "ReadableGlBuffer.h"

namespace Vixen::Engine {
    ReadableGlBuffer::ReadableGlBuffer(const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage)
            : GlBuffer(GL_MAP_READ_BIT, size, bufferUsage, allocationUsage) {}
}
