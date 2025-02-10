#pragma once

#include <cstdint>

#include "Bitmask.h"

namespace Vixen {
    enum class PipelineStageFlags : uint32_t {
        Top = 1 << 0,
        DrawIndirect = 1 << 1,
        VertexInput = 1 << 2,
        VertexShader = 1 << 3,
        TessellationControl = 1 << 4,
        TessellationEvaluation = 1 << 5,
        GeometryShader = 1 << 6,
        FragmentShader = 1 << 7,
        EarlyFragmentTests = 1 << 8,
        LateFragmentTests = 1 << 9,
        ColorAttachmentOutput = 1 << 10,
        ComputeShader = 1 << 11,
        Copy = 1 << 12,
        Bottom = 1 << 13,
        Resolve = 1 << 14,
        AllGraphics = 1 << 15,
        AllCommands = 1 << 16,
        ClearStorage = 1 << 17
    };

    DECLARE_BITMASK(PipelineStageFlags);
}
