#pragma once

namespace Vixen::Engine {
    struct ShaderModule {
        enum class Stage {
            VERTEX,
            FRAGMENT
        };

        Stage stage;

        explicit ShaderModule(Stage stage);
    };
}
