#pragma once

namespace Vixen {
    class Shader;
    class PipelineLayout;

    struct ComputePipelineDescription {
        const Shader* shader = nullptr;
        const PipelineLayout* layout = nullptr;
    };
}
