#pragma once

#include "../Vulkan.h"

namespace Vixen {
    class VulkanShaderProgram;
    class VulkanDevice;

    class VulkanPipelineLayout {
        std::shared_ptr<VulkanDevice> device;

        ::VkPipelineLayout layout = VK_NULL_HANDLE;

    public:
        VulkanPipelineLayout(const std::shared_ptr<VulkanDevice> &device, const VulkanShaderProgram &program);

        VulkanPipelineLayout(VulkanPipelineLayout& other) = delete;

        VulkanPipelineLayout& operator=(const VulkanPipelineLayout& other) = delete;

        VulkanPipelineLayout(VulkanPipelineLayout&& fp) noexcept;

        VulkanPipelineLayout& operator=(VulkanPipelineLayout&& fp) noexcept;

        ~VulkanPipelineLayout();

        [[nodiscard]] VkPipelineLayout getLayout() const;
    };
}
