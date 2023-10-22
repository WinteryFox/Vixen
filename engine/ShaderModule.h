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

        struct IO {
            std::string name;
            std::uint32_t size;
            std::optional<uint32_t> location;
            std::optional<uint32_t> binding;
        };

        ShaderModule(
                Stage stage,
                const std::vector<IO> &inputs,
                const std::vector<IO> &outputs,
                std::string entrypoint
        ) : stage(stage),
            inputs(inputs),
            outputs(outputs),
            entrypoint(std::move(entrypoint)) {}

        [[nodiscard]] Stage getStage() const {
            return stage;
        }

        [[nodiscard]] const std::string &getEntrypoint() const {
            return entrypoint;
        }

        [[nodiscard]] const std::vector<IO> &getInputs() const {
            return inputs;
        }

        [[nodiscard]] const std::vector<IO> &getOutputs() const {
            return outputs;
        }

    protected:
        Stage stage;

        std::string entrypoint;

        std::vector<IO> inputs;

        std::vector<IO> outputs;
    };
}
