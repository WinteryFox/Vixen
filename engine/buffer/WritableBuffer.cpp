#include "WritableBuffer.h"

namespace Vixen::Engine {
    WritableBuffer::WritableBuffer(const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage)
            : Buffer(size, bufferUsage, allocationUsage) {}
}
