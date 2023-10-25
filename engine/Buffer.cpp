#include "Buffer.h"

namespace Vixen::Vk {
    Buffer::Buffer(Usage bufferUsage, const size_t &size)
            : bufferUsage(bufferUsage),
              size(size) {}

    Buffer::Usage Buffer::getBufferUsage() const {
        return bufferUsage;
    }

    size_t Buffer::getSize() const {
        return size;
    }
}
