#pragma once

#include <vector>

#include "ShaderUniform.h"

namespace Vixen {
    enum class ShaderStage;

    struct Shader {
        std::string name;
        uint32_t pushConstantSize;
        std::vector<ShaderStage> pushConstantStages;
        std::vector<ShaderUniform> uniformSets;
        std::vector<ShaderStage> stages;

        virtual ~Shader() = default;
    };
}
