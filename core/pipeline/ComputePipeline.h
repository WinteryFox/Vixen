#pragma once

#include "Pipeline.h"

namespace Vixen {
    class ComputePipeline : public Pipeline {
    protected:
        explicit ComputePipeline(const PipelineLayout& layout);
    };
}
