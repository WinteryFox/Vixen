#pragma once

#include <utility>
#include <vector>
#include <string>
#include <utility>
#include <optional>
#include <cstdint>

namespace Vixen {
    class ShaderModule {
    public:
        enum class Stage {
            Vertex,
            Fragment
        };

        enum class Rate {
            Vertex,
            Instance
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
            enum class Type {
                Buffer,
                Sampler
            };

            Stage stage;
            std::optional<uint32_t> binding;
            Type type;
        };

    private:
        Stage stage;

        std::string entrypoint;

        std::vector<Binding> bindings;

        std::vector<IO> inputs;

        std::vector<Uniform> uniforms;

    public:
        ShaderModule(
            const Stage stage,
            std::string entrypoint,
            const std::vector<Binding>& bindings,
            const std::vector<IO>& inputs,
            const std::vector<Uniform>& uniforms
        ) : stage(stage),
            entrypoint(std::move(entrypoint)),
            bindings(bindings),
            inputs(inputs),
            uniforms(uniforms) {}

        ShaderModule(const ShaderModule&) = delete;

        ShaderModule& operator=(const ShaderModule&) = delete;

        ShaderModule(ShaderModule&&) = default;

        ShaderModule& operator=(ShaderModule&&) = default;

        virtual ~ShaderModule() = default;

        [[nodiscard]] Stage getStage() const {
            return stage;
        }

        [[nodiscard]] const std::string& getEntrypoint() const {
            return entrypoint;
        }

        [[nodiscard]] const std::vector<Binding>& getBindings() const {
            return bindings;
        }

        [[nodiscard]] const std::vector<IO>& getInputs() const {
            return inputs;
        }

        [[nodiscard]] std::vector<Uniform> getUniforms() const {
            return uniforms;
        }
    };
}
