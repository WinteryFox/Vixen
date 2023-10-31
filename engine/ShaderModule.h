#pragma once

#include <string>
#include <utility>
#include <optional>

namespace Vixen {
    class ShaderModule {
    public:
        enum class Stage {
            VERTEX,
            FRAGMENT,
        };

        enum class Rate {
            VERTEX,
            INSTANCE
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

    private:
        Stage stage;

        std::string entrypoint;

        std::vector<Binding> bindings;

        std::vector<IO> inputs;

        std::vector<IO> uniformBuffers;

    public:
        ShaderModule(
                Stage stage,
                std::string entrypoint,
                const std::vector<Binding> &bindings,
                const std::vector<IO> &inputs
        ) : stage(stage),
            entrypoint(std::move(entrypoint)),
            bindings(bindings),
            inputs(inputs) {}

        [[nodiscard]] Stage getStage() const {
            return stage;
        }

        [[nodiscard]] const std::string &getEntrypoint() const {
            return entrypoint;
        }

        [[nodiscard]] const std::vector<Binding> &getBindings() const {
            return bindings;
        }

        [[nodiscard]] const std::vector<IO> &getInputs() const {
            return inputs;
        }
    };
}
