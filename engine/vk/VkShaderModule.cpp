#include "VkShaderModule.h"

namespace Vixen::Vk {
    VkShaderModule::VkShaderModule(
            const std::shared_ptr<Device> &device,
            Stage stage,
            const std::vector<uint32_t> &binary,
            const std::vector<IO> &inputs,
            const std::vector<IO> &outputs,
            const std::string &entrypoint
    ) : ShaderModule(stage, inputs, outputs, entrypoint),
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

    ::VkShaderModule VkShaderModule::getModule() const {
        return module;
    }
}
