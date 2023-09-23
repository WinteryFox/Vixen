#pragma once

#include "Device.h"
#include "Swapchain.h"
#include "VkShaderProgram.h"

namespace Vixen::Vk {
    class VkRenderPass {
        std::shared_ptr<Device> device;

        ::VkRenderPass renderPass;

    public:
        VkRenderPass(
                const std::shared_ptr<Device> &device,
                const VkShaderProgram &program,
                const Swapchain &swapchain
        );

        VkRenderPass(const VkRenderPass &) = delete;

        VkRenderPass &operator=(const VkRenderPass &) = delete;

        ~VkRenderPass();

        [[nodiscard]] ::VkRenderPass getRenderPass() const;
    };
}
