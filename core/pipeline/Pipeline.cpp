#include "Pipeline.h"

#include "shader/Shader.h"

namespace Vixen {
    Pipeline::Pipeline(
        const PipelineLayout& layout,
        const Shader& shader
    ) : layout(layout),
        shaderRequirements{
            .descriptorBindings = shader.getUniformSets(),
            .pushConstants = shader.getPushConstantSize() != 0
                                 ? std::optional{
                                     PushConstantRange{
                                         .stages = shader.getPushConstantStages(),
                                         .offset = 0,
                                         .size = shader.getPushConstantSize()
                                     }
                                 }
                                 : std::nullopt
        } {}

    const PipelineLayout& Pipeline::getLayout() const noexcept {
        return layout;
    }
}
