#include "GraphicsPipeline.h"

namespace Vixen {
    GraphicsPipeline::GraphicsPipeline(
        const PipelineLayout& layout,
        GraphicsPipelineState state
    ) : Pipeline(layout),
        state(std::move(state)) {}
}
