#include "ReadableBuffer.h"

namespace Vixen::Engine {
    ReadableBuffer::ReadableBuffer(const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage)
            : Buffer(size, bufferUsage, allocationUsage) {}
}
