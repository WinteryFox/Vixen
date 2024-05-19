#include "VulkanShaderProgram.h"

#include "src/descriptorset/VulkanDescriptorSetLayout.h"

namespace Vixen {
    VulkanShaderProgram::VulkanShaderProgram(
        const std::shared_ptr<VulkanShaderModule>& vertex,
        const std::shared_ptr<VulkanShaderModule>& fragment
    ) : ShaderProgram(vertex, fragment),
        descriptorSetLayout(nullptr) {
        const auto& vertexBindings = vertex->createBindings();
        const auto& fragmentBindings = fragment->createBindings();

        std::vector<VkDescriptorSetLayoutBinding> bindings{};
        bindings.insert(bindings.end(), vertexBindings.begin(), vertexBindings.end());
        bindings.insert(bindings.end(), fragmentBindings.begin(), fragmentBindings.end());

        descriptorSetLayout = std::make_shared<VulkanDescriptorSetLayout>(vertex->getDevice(), bindings);
    }

    std::shared_ptr<VulkanDescriptorSetLayout> VulkanShaderProgram::getDescriptorSetLayout() const {
        return descriptorSetLayout;
    }
}
