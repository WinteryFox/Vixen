#pragma once

#include <string>
#include <utility>

namespace Vixen::Engine {
    class ShaderModule {
    public:
        enum class Stage {
            VERTEX,
            FRAGMENT,
        };

        ShaderModule(Stage stage, std::string entrypoint)
                : stage(stage), entrypoint(std::move(entrypoint)) {}

        [[nodiscard]] Stage getStage() const {
            return stage;
        }

        [[nodiscard]] const std::string &getEntrypoint() const {
            return entrypoint;
        }

    protected:
        Stage stage;

        std::string entrypoint;
    };
}
