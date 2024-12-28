#pragma once
#include "core/Bitmask.h"

namespace Vixen {
    enum class ImageUsage : int64_t {
        Sampling = 1 << 0,
        ColorAttachment = 1 << 1,
        DepthStencilAttachment = 1 << 2,
        Storage = 1 << 3,
        StorageAtomic = 1 << 4,
        CpuRead = 1 << 5,
        Update = 1 << 6,
        CopySource = 1 << 7,
        CopyDestination = 1 << 8,
        InputAttachment = 1 << 9,
        Transient = 1 << 10
    };

    DECLARE_BITMASK(ImageUsage);
}
