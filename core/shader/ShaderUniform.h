#pragma once

#include <cstdint>

#include "ShaderStage.h"
#include "ShaderUniformType.h"

namespace Vixen {
    struct ShaderUniform {
        ShaderUniformType type;
        uint32_t binding;
        std::vector<ShaderStage> stages;
        uint32_t length;
    };
}
