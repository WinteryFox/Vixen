#include "VkShaderProgram.h"

namespace Vixen::Vk {
    VkShaderProgram::VkShaderProgram(
        const std::shared_ptr<VkShaderModule>& vertex,
        const std::shared_ptr<VkShaderModule>& fragment
    ) : ShaderProgram(vertex, fragment),
        descriptorSetLayout(nullptr) {
        const auto& vertexBindings = vertex->createBindings();
        const auto& fragmentBindings = fragment->createBindings();

        std::vector<VkDescriptorSetLayoutBinding> bindings{};
        bindings.insert(bindings.end(), vertexBindings.begin(), vertexBindings.end());
        bindings.insert(bindings.end(), fragmentBindings.begin(), fragmentBindings.end());

        descriptorSetLayout = std::make_shared<VkDescriptorSetLayout>(vertex->getDevice(), bindings);
    }

    std::shared_ptr<VkDescriptorSetLayout> VkShaderProgram::getDescriptorSetLayout() const {
        return descriptorSetLayout;
    }
}
