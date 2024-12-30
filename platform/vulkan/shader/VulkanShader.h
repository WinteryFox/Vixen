#pragma once

#include <volk.h>

#include "core/shader/Shader.h"

namespace Vixen {
    struct VulkanShader : Shader {
        VkShaderModule module;
        VkPipelineLayout pipelineLayout;
        VkDescriptorSetLayout descriptorSetLayout;
    };
}
