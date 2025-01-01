#pragma once

#include <cstdint>

#include "ShaderStage.h"
#include "ShaderUniformType.h"

namespace Vixen {
    struct ShaderUniform {
        ShaderUniformType type;
        uint32_t binding;
        uint32_t length;
    };
}
