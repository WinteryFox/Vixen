#include "VulkanComputePipeline.h"

#include "VulkanPipelineLayout.h"

namespace Vixen {
    VulkanComputePipeline::VulkanComputePipeline(
        const VulkanPipelineLayout& layout,
        const VkPipeline pipeline
    ) : ComputePipeline(layout),
        pipeline(pipeline) {}
}
