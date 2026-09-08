#include "VulkanComputePipeline.h"

#include "VulkanPipelineLayout.h"

namespace Vixen {
    VulkanComputePipeline::VulkanComputePipeline(
        const VulkanPipelineLayout& layout,
        const Shader& shader,
        const VkPipeline pipeline
    ) : ComputePipeline(layout, shader),
        pipeline(pipeline) {}
}
