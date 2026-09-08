#pragma once

#include "Pipeline.h"

namespace Vixen {
    class ComputePipeline : public Pipeline {
    protected:
        ComputePipeline(const PipelineLayout& layout, const Shader& shader);
    };
}
