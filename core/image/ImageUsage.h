#pragma once

#include "core/Bitmask.h"

namespace Vixen {
    enum class ImageUsageBits : uint32_t {
        Sampling = 1u << 0,
        ColorAttachment = 1u << 1,
        DepthStencilAttachment = 1u << 2,
        Storage = 1u << 3,
        StorageAtomic = 1u << 4,
        CpuRead = 1u << 5,
        Update = 1u << 6,
        CopySource = 1u << 7,
        CopyDestination = 1u << 8,
        InputAttachment = 1u << 9,
        Transient = 1u << 10
    };

    template <>
    struct EnableFlags<ImageUsageBits> : std::true_type {};

    using ImageUsageFlags = Flags<ImageUsageBits>;
}
