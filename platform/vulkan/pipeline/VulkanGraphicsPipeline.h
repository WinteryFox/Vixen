#pragma once

#include <volk.h>

#include "pipeline/GraphicsPipeline.h"

namespace Vixen {
    class VulkanPipelineLayout;

    class VulkanGraphicsPipeline final : public GraphicsPipeline {
        friend class VulkanRenderingDeviceDriver;

        VkPipeline pipeline = VK_NULL_HANDLE;

    public:
        VulkanGraphicsPipeline(
            const VulkanPipelineLayout& layout,
            GraphicsPipelineState state,
            VkPipeline pipeline
        );
    };
}
