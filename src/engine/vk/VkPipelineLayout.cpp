#include "VkPipelineLayout.h"

namespace Vixen::Vk {

    VkPipelineLayout::VkPipelineLayout(const std::shared_ptr<Device> &device, const VkShaderProgram &program)
            : device(device) {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 0,
                .pSetLayouts = nullptr,
                .pushConstantRangeCount = 0,
                .pPushConstantRanges = nullptr,
        };

        Vk::checkVulkanResult(
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
