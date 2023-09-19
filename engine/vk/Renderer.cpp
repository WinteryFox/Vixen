#include "Renderer.h"

namespace Vixen::Vk {
    Renderer::Renderer(const std::shared_ptr<Vk::Device> &device,
                       std::unique_ptr<Vk::Pipeline> &&pipeline)
            : device(device), pipeline(std::move(pipeline)) {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 0,
                .pSetLayouts = nullptr,
                .pushConstantRangeCount = 0,
                .pPushConstantRanges = nullptr,
        };

        Vk::checkVulkanResult(
                vkCreatePipelineLayout(device->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout),
                "Failed to create pipeline layout"
        );
    }

    Renderer::~Renderer() {
        vkDestroyPipelineLayout(device->getDevice(), pipelineLayout, nullptr);
    }
}
