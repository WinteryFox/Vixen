#pragma once

#include <vector>

#include "ShaderStage.h"

namespace Vixen {
    struct ShaderStageData {
        ShaderStage stage;
        std::vector<std::byte> spirv;
    };
}
