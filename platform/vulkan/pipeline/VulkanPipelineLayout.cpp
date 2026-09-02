#include "VulkanPipelineLayout.h"

namespace Vixen {
    VulkanPipelineLayout::VulkanPipelineLayout(
        PipelineLayoutDescription description,
        const VkPipelineLayout layout,
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts
    ) : PipelineLayout(std::move(description)),
        layout(layout),
        descriptorSetLayouts(std::move(descriptorSetLayouts)) {}
}
