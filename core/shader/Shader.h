#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ShaderStage.h"
#include "ShaderUniform.h"

namespace Vixen {
    class RenderingDeviceDriver;

    class Shader {
        friend class RenderingDeviceDriver;

        uint32_t pushConstantSize = 0;
        ShaderStageFlags pushConstantStages{};
        std::vector<ShaderUniform> uniformSets;
        ShaderStageFlags stages{};

    protected:
        Shader() = default;

    public:
        std::string name;

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&&) = delete;
        Shader& operator=(Shader&&) = delete;

        virtual ~Shader() = default;

        [[nodiscard]] uint32_t getPushConstantSize() const noexcept {
            return pushConstantSize;
        }

        [[nodiscard]] ShaderStageFlags getPushConstantStages() const noexcept {
            return pushConstantStages;
        }

        [[nodiscard]] const std::vector<ShaderUniform>& getUniformSets() const noexcept {
            return uniformSets;
        }

        [[nodiscard]] ShaderStageFlags getStageFlags() const noexcept {
            return stages;
        }
    };
}
