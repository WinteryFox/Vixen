#pragma once

#include "ShaderUniform.h"

namespace Vixen {
    struct Shader {
        std::string name;
        uint32_t pushConstantSize;
        std::vector<ShaderUniform> uniforms;
        std::vector<ShaderStage> stages;
        uint32_t vertexInputMask;
        uint32_t fragmentOutputMask;
    };
}
