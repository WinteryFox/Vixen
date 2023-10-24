#include "Buffer.h"

namespace Vixen::Vk {
    Buffer::Buffer(BufferUsage bufferUsage, const size_t &size)
            : bufferUsage(bufferUsage),
              size(size) {}

    BufferUsage Buffer::getBufferUsage() const {
        return bufferUsage;
    }

    size_t Buffer::getSize() const {
        return size;
    }
}
