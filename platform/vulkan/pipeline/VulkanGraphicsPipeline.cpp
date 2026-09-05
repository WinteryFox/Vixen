#include "VulkanGraphicsPipeline.h"

#include "VulkanPipelineLayout.h"

namespace Vixen {
    VulkanGraphicsPipeline::VulkanGraphicsPipeline(
        const VulkanPipelineLayout& layout,
        GraphicsPipelineState state,
        VkPipeline pipeline
    ) : GraphicsPipeline(layout, std::move(state)),
        pipeline(pipeline) {}
}
