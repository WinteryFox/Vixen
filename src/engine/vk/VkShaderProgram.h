#pragma once

#include "../ShaderProgram.h"
#include "VkShaderModule.h"
#include "VkDescriptorSetLayout.h"

namespace Vixen::Vk {
    class VkShaderProgram : public ShaderProgram<VkShaderModule> {
        std::shared_ptr<VkDescriptorSetLayout> descriptorSetLayout;

    public:
        VkShaderProgram(
            const std::shared_ptr<VkShaderModule>& vertex,
            const std::shared_ptr<VkShaderModule>& fragment
        );

        [[nodiscard]] std::shared_ptr<VkDescriptorSetLayout> getDescriptorSetLayout() const;
    };
}
