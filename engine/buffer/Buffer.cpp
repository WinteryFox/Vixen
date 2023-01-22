#include "Buffer.h"

namespace Vixen::Engine {
    Buffer::Buffer(const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage)
            : size(size), bufferUsage(bufferUsage), allocationUsage(allocationUsage) {}
}
