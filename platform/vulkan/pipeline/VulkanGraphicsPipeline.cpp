#include "VulkanGraphicsPipeline.h"

#include "VulkanPipelineLayout.h"

namespace Vixen {
    VulkanGraphicsPipeline::VulkanGraphicsPipeline(
        const VulkanPipelineLayout& layout,
        VkPipeline pipeline
    ) : GraphicsPipeline(layout),
        pipeline(pipeline) {}
}
