#include "ReadableBuffer.h"

namespace Vixen::Vk {
    ReadableBuffer::ReadableBuffer(const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage)
            : Buffer(size, bufferUsage, allocationUsage) {}
}
