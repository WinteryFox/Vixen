#pragma once

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

        enum class Type {
            Buffer,
            Sampler
        };

        struct Binding {
            uint32_t binding;
            size_t stride;
            Rate rate;
        };

        struct IO {
            std::optional<uint32_t> binding;
            std::optional<uint32_t> location;
            size_t size;
            size_t offset;
        };

        struct Uniform {
            std::optional<uint32_t> binding{};
            Type type;
        };

        struct PushConstant {
            Stage stage;
            uint32_t offset;
            uint32_t size;
        };

        std::vector<Binding> bindings{};

        std::vector<IO> inputs{};

        std::vector<Uniform> uniforms{};

        std::vector<PushConstant> pushConstants{};
    };
}
