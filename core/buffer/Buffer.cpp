#include "Buffer.h"

namespace Vixen {
    Buffer::Buffer(
        const BufferUsageFlags usage,
        const uint32_t count,
        const uint32_t stride
    ) : usage(usage),
        count(count),
        stride(stride) {
    }

    BufferUsageFlags Buffer::getUsage() const {
        return usage;
    }

    uint32_t Buffer::getCount() const {
        return count;
    }

    uint32_t Buffer::getStride() const {
        return stride;
    }

    uint64_t Buffer::getSize() const {
        return static_cast<uint64_t>(count) * stride;
    }
}
