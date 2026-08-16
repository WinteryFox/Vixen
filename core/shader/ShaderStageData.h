#pragma once

#include <vector>

#include "ShaderStage.h"

namespace Vixen {
    struct ShaderStageData {
        ShaderStageBits stage;
        std::vector<std::byte> spirv;
    };
}
