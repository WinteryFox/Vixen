#include "Buffer.h"

namespace Vixen {
    Buffer::Buffer(
        const BufferUsage usage,
        const uint32_t count,
        const uint32_t stride
    ) : usage(usage),
        count(count),
        stride(stride) {
    }

    BufferUsage Buffer::getUsage() const {
        return usage;
    }

    uint32_t Buffer::getCount() const {
        return count;
    }

    uint32_t Buffer::getStride() const {
        return stride;
    }

    uint64_t Buffer::getSize() const {
        return count * stride;
    }
}
