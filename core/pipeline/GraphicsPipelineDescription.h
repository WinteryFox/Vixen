#pragma once
#include "GraphicsPipelineState.h"

namespace Vixen {
    class Shader;
    class PipelineLayout;

    struct GraphicsPipelineDescription {
        const Shader* shader = nullptr;
        const PipelineLayout* layout = nullptr;

        GraphicsPipelineState state;
    };
}
