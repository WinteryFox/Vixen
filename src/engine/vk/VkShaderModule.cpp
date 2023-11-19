#include "VkShaderModule.h"

namespace Vixen::Vk {
    VkShaderModule::VkShaderModule(
        const std::shared_ptr<Device>& device,
        const Stage stage,
        const std::vector<uint32_t>& binary,
        const std::vector<Binding>& bindings,
        const std::vector<IO>& inputs,
        const std::string& entrypoint
    ) : ShaderModule(stage, entrypoint, bindings, inputs),
        device(device),
        module(VK_NULL_HANDLE) {
        const VkShaderModuleCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = binary.size() * sizeof(uint32_t),
            .pCode = binary.data()
        };

        checkVulkanResult(
            vkCreateShaderModule(device->getDevice(), &info, nullptr, &module),
            "Failed to create shader module"
        );
    }

    VkShaderModule::~VkShaderModule() {
        vkDestroyShaderModule(device->getDevice(), module, nullptr);
    }

    VkPipelineShaderStageCreateInfo VkShaderModule::createInfo() const {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .flags = 0,
            .stage = getVulkanShaderStage(getStage()),
            .module = module,
            .pName = getEntrypoint().c_str(),
            .pSpecializationInfo = VK_NULL_HANDLE
        };
    }

    std::vector<VkDescriptorSetLayoutBinding> VkShaderModule::createBindings() const {
        const auto& bindings = getBindings();

        std::vector<VkDescriptorSetLayoutBinding> b{bindings.size()};
        for (const auto& [binding, stride, rate] : bindings) {
            const VkShaderStageFlags& stage = getVulkanShaderStage(getStage());

            b.push_back(
                VkDescriptorSetLayoutBinding{
                    .binding = binding,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    // TODO: This count value is used for array bindings
                    .descriptorCount = 1,
                    .stageFlags = stage,
                    .pImmutableSamplers = nullptr
                }
            );
        }

        return b;
    }
}
