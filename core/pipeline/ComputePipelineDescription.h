#pragma once

namespace Vixen {
    struct Shader;
    class PipelineLayout;

    struct ComputePipelineDescription {
        const Shader* shader;
        const PipelineLayout* layout;
    };
}
