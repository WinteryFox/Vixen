#pragma once

namespace Vixen {
    enum ImageUsage {
        Sampling,
        ColorAttachment,
        DepthStencilAttachment,
        Storage,
        StorageAtomic,
        CpuRead,
        Update,
        CopySource,
        CopyDestination,
        InputAttachment,
        Transient
    };
}
