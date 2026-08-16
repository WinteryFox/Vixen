#pragma once

#include <vector>

#include "ShaderUniform.h"

namespace Vixen {
    struct Shader {
        std::string name;
        uint32_t pushConstantSize;
        ShaderStageFlags pushConstantStages;
        std::vector<ShaderUniform> uniformSets;
        ShaderStageFlags stages;

        virtual ~Shader() = default;
    };
}
