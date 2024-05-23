#pragma once

#include <vector>
#include <cstdint>

namespace Vixen {
    struct ShaderResources {
        enum class Stage {
            All,
            Vertex,
            Fragment
        };

        enum class Rate {
            Vertex,
            Instance
        };

        enum class PrimitiveType {
            Float1,
            Float2,
            Float3,
            Float4
        };

        enum class UniformType {
            Buffer,
            Sampler
        };

        struct Binding {
            uint32_t binding;
            size_t stride;
            Rate rate;
        };

        struct Input {
            uint32_t binding;
            uint32_t location;
            PrimitiveType type;
            size_t offset;
        };

        struct Uniform {
            uint32_t binding;
            UniformType type;
        };

        struct PushConstant {
            Stage stage;
            uint32_t offset;
            uint32_t size;
        };

        std::vector<Uniform> uniforms{};

        std::vector<PushConstant> pushConstants{};
    };
}
