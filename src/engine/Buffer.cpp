#include "Buffer.h"

namespace Vixen {
    Buffer::Buffer(const Usage bufferUsage, const std::size_t &size)
            : bufferUsage(bufferUsage),
              size(size) {}

    Buffer::Usage Buffer::getBufferUsage() const {
        return bufferUsage;
    }

    std::size_t Buffer::getSize() const {
        return size;
    }
}
