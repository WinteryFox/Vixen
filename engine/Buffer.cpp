#include "Buffer.h"

namespace Vixen::Vk {
    Buffer::Buffer(const size_t &size, BufferUsage bufferUsage)
            : size(size), bufferUsage(bufferUsage) {}

    size_t Buffer::getSize() const {
        return size;
    }

    BufferUsage Buffer::getBufferUsage() const {
        return bufferUsage;
    }
}
