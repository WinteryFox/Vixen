#pragma once
#include <cstdint>

#include "core/Bitmask.h"

namespace Vixen {
    enum class ShaderStage {
        Vertex,
        Fragment,
        TesselationControl,
        TesselationEvaluation,
        Compute,
        Geometry
    };
}
