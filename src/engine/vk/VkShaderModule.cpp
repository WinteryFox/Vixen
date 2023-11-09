#include "VkShaderModule.h"

namespace Vixen::Vk {
    VkShaderModule::VkShaderModule(
            const std::shared_ptr<Device> &device,
            Stage stage,
            const std::vector<uint32_t> &binary,
            const std::vector<Binding> &bindings,
            const std::vector<IO> &inputs,
            const std::string &entrypoint
    ) : ShaderModule(stage, entrypoint, bindings, inputs),
        device(device),
        module(VK_NULL_HANDLE) {
        VkShaderModuleCreateInfo info{
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

    VkPipelineShaderStageCreateInfo VkShaderModule::createInfo() {
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
}
