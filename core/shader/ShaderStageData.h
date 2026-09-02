#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "ShaderStage.h"

namespace Vixen {
    struct ShaderStageData {
        ShaderStageBits stage;
        std::vector<std::byte> spirv;
        std::string entryPoint = "main";
    };
}
