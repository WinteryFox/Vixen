#pragma once

#include <cstdint>

#include "Bitmask.h"

namespace Vixen {
    enum class PipelineStageBits : uint32_t {
        Top = 1u << 0,
        DrawIndirect = 1u << 1,
        VertexInput = 1u << 2,
        VertexShader = 1u << 3,
        TessellationControl = 1u << 4,
        TessellationEvaluation = 1u << 5,
        GeometryShader = 1u << 6,
        FragmentShader = 1u << 7,
        EarlyFragmentTests = 1u << 8,
        LateFragmentTests = 1u << 9,
        ColorAttachmentOutput = 1u << 10,
        ComputeShader = 1u << 11,
        Copy = 1u << 12,
        Bottom = 1u << 13,
        Resolve = 1u << 14,
        AllGraphics = 1u << 15,
        AllCommands = 1u << 16,
        ClearStorage = 1u << 17
    };

    using PipelineStageFlags = Flags<PipelineStageBits>;
} // namespace Vixen
