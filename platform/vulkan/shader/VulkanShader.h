#pragma once

#include <string>
#include <vector>
#include <volk.h>

#include "core/shader/Shader.h"

namespace Vixen {
    struct VulkanShaderModule {
        VkShaderStageFlagBits stage{};
        VkShaderModule module = VK_NULL_HANDLE;
        std::string entryPoint;
    };

    struct VulkanShader : Shader {
        std::vector<VulkanShaderModule> shaderModules;
    };
}
