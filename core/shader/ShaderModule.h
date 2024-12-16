#pragma once

#include <utility>
#include <string>
#include <utility>

#include "ShaderResources.h"

namespace Vixen {
    class ShaderModule {
        ShaderResources::Stage stage;

        std::string entrypoint;

        ShaderResources resources;

    public:
        ShaderModule(
            const ShaderResources::Stage stage,
            std::string entrypoint,
            ShaderResources resources
        ) : stage(stage),
            entrypoint(std::move(entrypoint)),
            resources(std::move(resources)) {}

        ShaderModule(const ShaderModule &) = delete;

        ShaderModule &operator=(const ShaderModule &) = delete;

        ShaderModule(ShaderModule &&) = default;

        ShaderModule &operator=(ShaderModule &&) = default;

        virtual ~ShaderModule() = default;

        [[nodiscard]] ShaderResources::Stage getStage() const { return stage; }

        [[nodiscard]] const std::string &getEntrypoint() const { return entrypoint; }

        [[nodiscard]] const ShaderResources &getResources() const { return resources; }
    };
}
