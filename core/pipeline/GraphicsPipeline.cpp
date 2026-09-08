#include "GraphicsPipeline.h"

#include <algorithm>
#include <utility>

namespace Vixen {
    auto GraphicsPipeline::buildVertexValidationRequirements(const GraphicsPipelineState& state)
        -> std::vector<VertexValidationRequirement> {
        std::vector<VertexValidationRequirement> requirements;
        requirements.reserve(state.vertexAttributes.size());

        for (const auto& attribute : state.vertexAttributes) {
            const auto binding = std::ranges::find(
                state.vertexBindings, attribute.binding, &VertexBindingDescription::binding
            );
            requirements.push_back({
                .location = attribute.location,
                .binding = attribute.binding,
                .attributeEnd = static_cast<uint64_t>(attribute.offset) + getTexelSize(attribute.format),
                .bindingDescription = binding != state.vertexBindings.end()
                                          ? std::optional{*binding}
                                          : std::nullopt
            });
        }

        return requirements;
    }

    GraphicsPipeline::GraphicsPipeline(
        const PipelineLayout& layout,
        const Shader& shader,
        GraphicsPipelineState state
    ) : Pipeline(layout, shader),
        state(std::move(state)),
        vertexValidationRequirements(buildVertexValidationRequirements(this->state)) {}
}
