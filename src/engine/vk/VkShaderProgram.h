#pragma once

#include "../ShaderProgram.h"
#include "VkShaderModule.h"

namespace Vixen::Vk {
    class VkShaderProgram : public ShaderProgram<VkShaderModule> {
    public:
        VkShaderProgram(const std::shared_ptr<VkShaderModule> &vertex, const std::shared_ptr<VkShaderModule> &fragment);
    };
}
