#include "VkPipelineLayout.h"

namespace Vixen::Vk {
    VkPipelineLayout::VkPipelineLayout(const std::shared_ptr<Device>& device, const VkShaderProgram& program)
        : device(device) {
        const auto& l = program.getVertex()->getDescriptorSetLayout().getLayout();
        const VkPipelineLayoutCreateInfo pipelineLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 1,
            .pSetLayouts = &l,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr
        };

        checkVulkanResult(
            vkCreatePipelineLayout(device->getDevice(), &pipelineLayoutInfo, nullptr, &layout),
            "Failed to create pipeline layout"
        );
    }

    VkPipelineLayout::~VkPipelineLayout() {
        vkDestroyPipelineLayout(device->getDevice(), layout, nullptr);
    }

    ::VkPipelineLayout VkPipelineLayout::getLayout() const {
        return layout;
    }
}
