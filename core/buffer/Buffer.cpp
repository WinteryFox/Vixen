#include "Buffer.h"

namespace Vixen {
    Buffer::Buffer(
        const BufferUsageFlags usage,
        const uint64_t size
    ) : usage(usage),
        size(size) {
    }

    BufferUsageFlags Buffer::getUsage() const {
        return usage;
    }

    uint64_t Buffer::getSize() const {
        return size;
    }
}
