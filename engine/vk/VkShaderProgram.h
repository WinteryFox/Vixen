#pragma once

#include <volk.h>
#include "../ShaderProgram.h"

namespace Vixen::Engine {
    class VkShaderProgram : ShaderProgram {
    public:
        explicit VkShaderProgram(const std::vector<std::shared_ptr<ShaderModule>> &modules);
    };
}
