#pragma once

#include <volk.h>

#include "pipeline/ComputePipeline.h"

namespace Vixen {
    class VulkanPipelineLayout;

    class VulkanComputePipeline final : public ComputePipeline {
        friend class VulkanRenderingDeviceDriver;

        VkPipeline pipeline = VK_NULL_HANDLE;

    public:
        VulkanComputePipeline(
            const VulkanPipelineLayout& layout,
            VkPipeline pipeline
        );
    };
}
