#pragma once

#include <optional>
#include <vector>

#include "PipelineLayout.h"
#include "shader/ShaderUniform.h"

namespace Vixen {
    class Shader;

    class Pipeline {
        friend class RenderingDeviceDriver;

        struct ShaderRequirements {
            std::vector<ShaderUniform> descriptorBindings;
            std::optional<PushConstantRange> pushConstants;
        };

        const PipelineLayout& layout;
        const ShaderRequirements shaderRequirements;

    protected:
        Pipeline(const PipelineLayout& layout, const Shader& shader);

    public:
        Pipeline(const Pipeline& other) = delete;
        Pipeline& operator=(const Pipeline& other) = delete;

        Pipeline(Pipeline&& other) noexcept = delete;
        Pipeline& operator=(Pipeline&& other) noexcept = delete;

        virtual ~Pipeline() = default;

        [[nodiscard]]
        const PipelineLayout& getLayout() const noexcept;
    };
}
