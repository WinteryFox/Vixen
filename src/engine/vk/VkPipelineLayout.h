#pragma once

#include "VkShaderProgram.h"

namespace Vixen::Vk {
    class VkPipelineLayout {
        std::shared_ptr<Device> device;

        ::VkPipelineLayout layout = VK_NULL_HANDLE;

    public:
        VkPipelineLayout(const std::shared_ptr<Device> &device, const VkShaderProgram &program);

        VkPipelineLayout(const VkPipelineLayout &) = delete;

        VkPipelineLayout &operator=(const VkPipelineLayout &) = delete;

        ~VkPipelineLayout();

        [[nodiscard]] ::VkPipelineLayout getLayout() const;
    };
}
