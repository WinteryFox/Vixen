#pragma once

#include <cstdint>

#include "Bitmask.h"

namespace Vixen {
    enum class BarrierAccessBits : uint32_t {
        IndirectCommandsRead = 1u << 0,
        IndexRead = 1u << 1,
        VertexAttributeRead = 1u << 2,
        UniformRead = 1u << 3,
        InputAttachmentRead = 1u << 4,
        ShaderRead = 1u << 5,
        ShaderWrite = 1u << 6,
        ColorAttachmentRead = 1u << 7,
        ColorAttachmentWrite = 1u << 8,
        DepthStencilAttachmentRead = 1u << 9,
        DepthStencilAttachmentWrite = 1u << 10,
        CopyRead = 1u << 11,
        CopyWrite = 1u << 12,
        HostRead = 1u << 13,
        HostWrite = 1u << 14,
        MemoryRead = 1u << 15,
        MemoryWrite = 1u << 16,
        FragmentShadingRateAttachmentRead = 1u << 17,
        ResolveRead = 1u << 18,
        ResolveWrite = 1u << 19,
        StorageClear = 1u << 20
    };

    template <>
    struct EnableFlags<BarrierAccessBits> : std::true_type {};

    using BarrierAccessFlags = Flags<BarrierAccessBits>;
}
