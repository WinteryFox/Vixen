#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "GraphicsPipelineState.h"
#include "Pipeline.h"

namespace Vixen {
    class GraphicsPipeline : public Pipeline {
        friend class RenderingDeviceDriver;
        friend class VulkanRenderingDeviceDriver;

        struct VertexValidationRequirement {
            uint32_t location;
            uint32_t binding;
            uint64_t attributeEnd;
            // Preserve the draw-time diagnostic for an undeclared binding.
            std::optional<VertexBindingDescription> bindingDescription;
        };

        static auto buildVertexValidationRequirements(const GraphicsPipelineState& state)
            -> std::vector<VertexValidationRequirement>;

        const GraphicsPipelineState state;
        const std::vector<VertexValidationRequirement> vertexValidationRequirements;

    protected:
        GraphicsPipeline(
            const PipelineLayout& layout,
            const Shader& shader,
            GraphicsPipelineState state
        );
    };
}
