#pragma once

#include <vector>
#include <volk.h>

#include "core/shader/Shader.h"

namespace Vixen {
    struct VulkanShader : Shader {
        VkShaderStageFlags pushConstantStageFlags = 0;
        std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    };
}
