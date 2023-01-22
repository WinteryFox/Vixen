#include "ReadableWritableBuffer.h"

namespace Vixen::Engine {
    ReadableWritableBuffer::ReadableWritableBuffer(const size_t &size, BufferUsage bufferUsage,
                                                   AllocationUsage allocationUsage)
            : ReadableBuffer(size, bufferUsage, allocationUsage),
              WritableBuffer(size, bufferUsage, allocationUsage) {}
}
