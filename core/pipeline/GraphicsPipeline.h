#pragma once

#include "GraphicsPipelineState.h"
#include "Pipeline.h"

namespace Vixen {
    class GraphicsPipeline : public Pipeline {
        friend class RenderingDeviceDriver;
        friend class VulkanRenderingDeviceDriver;

        const GraphicsPipelineState state;

    protected:
        GraphicsPipeline(
            const PipelineLayout& layout,
            GraphicsPipelineState state
        );
    };
}
