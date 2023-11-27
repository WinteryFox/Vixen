#include "VkShaderModule.h"

namespace Vixen::Vk {
    VkShaderModule::VkShaderModule(
        const std::shared_ptr<Device>& device,
        const Stage stage,
        const std::vector<uint32_t>& binary,
        const std::vector<Binding>& bindings,
        const std::vector<IO>& inputs,
        const std::vector<Uniform>& uniformBuffers,
        const std::string& entrypoint
    ) : ShaderModule(stage, entrypoint, bindings, inputs, uniformBuffers),
        device(device),
        module(VK_NULL_HANDLE),
        descriptorSetLayout(device, createBindings()) {
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
        const VkShaderStageFlags& stage = getVulkanShaderStage(getStage());
        const auto& uniformBuffers = getUniforms();

        std::vector<VkDescriptorSetLayoutBinding> b{};
        b.reserve(uniformBuffers.size());
        for (const auto& uniformBuffer : uniformBuffers) {
            VkDescriptorType type;
            switch (uniformBuffer.type) {
            case Uniform::Type::BUFFER:
                type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                break;
            case Uniform::Type::SAMPLER:
                type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                break;
            default:
                throw std::runtime_error("Unknown uniform type");
            }

            b.emplace_back(
                VkDescriptorSetLayoutBinding{
                    .binding = uniformBuffer.binding.value_or(0),
                    .descriptorType = type,
                    // TODO: This count value is used for array bindings
                    .descriptorCount = 1,
                    .stageFlags = stage,
                    .pImmutableSamplers = nullptr
                }
            );
        }

        return b;
    }

    const VkDescriptorSetLayout& VkShaderModule::getDescriptorSetLayout() {
        return descriptorSetLayout;
    }
}
