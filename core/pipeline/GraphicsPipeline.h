#pragma once

#include "GraphicsPipelineState.h"
#include "Pipeline.h"

namespace Vixen {
    class GraphicsPipeline : public Pipeline {
        const GraphicsPipelineState state;

    protected:
        GraphicsPipeline(
            const PipelineLayout& layout,
            GraphicsPipelineState state
        );
    };
}
