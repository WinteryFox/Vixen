#pragma once

#include <cstdint>
#include <vector>

#include "shader/ShaderStage.h"
#include "shader/ShaderUniformType.h"

namespace Vixen {
    struct Sampler;
    struct Image;

    struct DescriptorBindingLayout {
        uint32_t binding;
        ShaderUniformType type;
        uint32_t count;
        ShaderStageFlags stages;
        std::vector<const Sampler*> immutableSamplers;

        friend bool operator==(
            const DescriptorBindingLayout& lhs,
            const DescriptorBindingLayout& rhs
        ) = default;
    };

    struct DescriptorSetLayoutDescription {
        uint32_t set;
        std::vector<DescriptorBindingLayout> bindings;
    };

    struct PushConstantRange {
        ShaderStageFlags stages;
        uint32_t offset;
        uint32_t size;
    };

    struct PipelineLayoutDescription {
        std::vector<DescriptorSetLayoutDescription> descriptorSets;
        std::vector<PushConstantRange> pushConstantRanges;
    };

    class PipelineLayout {
        PipelineLayoutDescription description;

    protected:
        explicit PipelineLayout(PipelineLayoutDescription description);

    public:
        PipelineLayout(const PipelineLayout& other) = delete;
        PipelineLayout& operator=(const PipelineLayout& other) = delete;

        PipelineLayout(PipelineLayout&& other) noexcept = delete;
        PipelineLayout& operator=(PipelineLayout&& other) noexcept = delete;

        virtual ~PipelineLayout() = default;

        [[nodiscard]]
        const PipelineLayoutDescription& getDescription() const noexcept;
    };
}
