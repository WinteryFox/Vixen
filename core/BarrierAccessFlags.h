#pragma once
#include <cstdint>

#include "Bitmask.h"

namespace Vixen {
    enum class BarrierAccessFlags : uint32_t {
        IndirectCommandsRead = 1 << 0,
        IndexRead = 1 << 1,
        VertexAttributeRead = 1 << 2,
        UniformRead = 1 << 3,
        InputAttachmentRead = 1 << 4,
        ShaderRead = 1 << 5,
        ShaderWrite = 1 << 6,
        ColorAttachmentRead = 1 << 7,
        ColorAttachmentWrite = 1 << 8,
        DepthStencilAttachmentRead = 1 << 9,
        DepthStencilAttachmentWrite = 1 << 10,
        CopyRead = 1 << 11,
        CopyWrite = 1 << 12,
        HostRead = 1 << 13,
        HostWrite = 1 << 14,
        MemoryRead = 1 << 15,
        MemoryWrite = 1 << 16,
        FragmentShadingRateAttachmentRead = 1 << 17,
        ResolveRead = 1 << 18,
        ResolveWrite = 1 << 19,
        StorageClear = 1 << 20
    };

    DECLARE_BITMASK(BarrierAccessFlags);
}
