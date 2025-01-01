#pragma once

#include <volk.h>

#include "core/shader/Shader.h"

namespace Vixen {
    struct VulkanShader : Shader {
        VkShaderStageFlags pushConstantStageFlags;
        std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        VkPipelineLayout pipelineLayout;
    };
}
