#include "VulkanGraphicsPipeline.h"

#include "VulkanPipelineLayout.h"

namespace Vixen {
    VulkanGraphicsPipeline::VulkanGraphicsPipeline(
        const VulkanPipelineLayout& layout,
        const Shader& shader,
        GraphicsPipelineState state,
        VkPipeline pipeline
    ) : GraphicsPipeline(layout, shader, std::move(state)),
        pipeline(pipeline) {}
}
