#pragma once

#include <vector>
#include <cstddef>

#include "ShaderStage.h"

namespace Vixen {
    struct ShaderStageData {
        ShaderStageBits stage;
        std::vector<std::byte> spirv;
    };
}
