#pragma once

#include "Pipeline.h"

namespace Vixen {
    class GraphicsPipeline : public Pipeline {
    protected:
        explicit GraphicsPipeline(const PipelineLayout& layout);
    };
}
