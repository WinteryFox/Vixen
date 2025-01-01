#pragma once

#include <volk.h>

#include "core/shader/Shader.h"

namespace Vixen {
    struct VulkanShader : Shader {
        VkShaderStageFlags pushConstantStageFlags;
        VkShaderModule module;
        VkPipelineLayout pipelineLayout;
        std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    };
}
