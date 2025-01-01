#pragma once

#include "ShaderUniform.h"

namespace Vixen {
    struct Shader {
        uint32_t pushConstantSize;
        std::vector<ShaderUniform> uniformSets;
        std::vector<ShaderStage> stages;
    };
}
