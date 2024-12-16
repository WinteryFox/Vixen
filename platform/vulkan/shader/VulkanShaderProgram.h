#pragma once

#include <memory>

#include "core/shader/ShaderProgram.h"
#include "VulkanShaderModule.h"

namespace Vixen {
    class VulkanDescriptorSetLayout;

    class VulkanShaderProgram : public ShaderProgram<VulkanShaderModule> {
        std::shared_ptr<VulkanDescriptorSetLayout> descriptorSetLayout;

    public:
        VulkanShaderProgram(
            const std::shared_ptr<VulkanShaderModule>& vertex,
            const std::shared_ptr<VulkanShaderModule>& fragment
        );

        [[nodiscard]] std::shared_ptr<VulkanDescriptorSetLayout> getDescriptorSetLayout() const;
    };
}
