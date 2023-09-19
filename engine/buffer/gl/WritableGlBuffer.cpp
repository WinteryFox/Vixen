#include "WritableGlBuffer.h"

namespace Vixen::Vk {
    WritableGlBuffer::WritableGlBuffer(const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage)
            : GlBuffer(GL_MAP_WRITE_BIT, size, bufferUsage, allocationUsage),
              WritableBuffer(size, bufferUsage, allocationUsage) {}

    WritableGlBuffer &WritableGlBuffer::write(const void *data, std::size_t dataSize, std::size_t offset) {
        std::memcpy(static_cast<char*>(dataPointer) + offset, data, dataSize);
        return *this;
    }
}
