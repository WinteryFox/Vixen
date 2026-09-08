#include "ComputePipeline.h"

namespace Vixen {
    ComputePipeline::ComputePipeline(
        const PipelineLayout& layout,
        const Shader& shader
    ) : Pipeline(layout, shader) {}
}
