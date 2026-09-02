#pragma once

#include <volk.h>

#include "pipeline/PipelineLayout.h"

namespace Vixen {
    class VulkanPipelineLayout final : public PipelineLayout {
        friend class VulkanRenderingDeviceDriver;

        VkPipelineLayout layout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

    public:
        VulkanPipelineLayout(
            PipelineLayoutDescription description,
            VkPipelineLayout layout,
            std::vector<VkDescriptorSetLayout> descriptorSetLayouts
        );
    };
}
