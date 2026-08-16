#pragma once

#include <cstdint>

#include "Bitmask.h"

namespace Vixen {
    enum class ShaderStageBits : uint32_t {
        Vertex = 1u << 0,
        Fragment = 1u << 1,
        TesselationControl = 1u << 2,
        TesselationEvaluation = 1u << 3,
        Compute = 1u << 4,
        Geometry = 1u << 5
    };

    template <>
    struct EnableFlags<ShaderStageBits> : std::true_type {};

    using ShaderStageFlags = Flags<ShaderStageBits>;
}
