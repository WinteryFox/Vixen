#pragma once

#include "VkShaderProgram.h"

namespace Vixen::Vk {
    class VkPipelineLayout {
        std::shared_ptr<Device> device;

        ::VkPipelineLayout layout = VK_NULL_HANDLE;

    public:
        VkPipelineLayout(const std::shared_ptr<Device> &device, const VkShaderProgram &program);

        VkPipelineLayout(VkPipelineLayout& other) = delete;

        VkPipelineLayout& operator=(const VkPipelineLayout& other) = delete;

        VkPipelineLayout(VkPipelineLayout&& fp) noexcept;

        VkPipelineLayout& operator=(VkPipelineLayout&& fp) noexcept;

        ~VkPipelineLayout();

        [[nodiscard]] ::VkPipelineLayout getLayout() const;
    };
}
