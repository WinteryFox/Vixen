#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ShaderStage.h"
#include "ShaderUniform.h"

namespace Vixen {
    struct Shader {
        std::string name;
        uint32_t pushConstantSize = 0;
        ShaderStageFlags pushConstantStages{};
        std::vector<ShaderUniform> uniformSets;
        ShaderStageFlags stages{};

        virtual ~Shader() = default;
    };
}
