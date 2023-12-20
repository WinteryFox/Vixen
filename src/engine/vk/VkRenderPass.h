#pragma once

#include "Device.h"
#include "Swapchain.h"
#include "VkShaderProgram.h"

namespace Vixen::Vk {
    class VkRenderPass {
        std::shared_ptr<Device> device;

        ::VkRenderPass renderPass;

        std::vector<VkAttachmentDescription> attachments;

        std::vector<VkAttachmentReference> references;

    public:
        VkRenderPass(
                const std::shared_ptr<Device> &device,
                VkFormat format
        );

        VkRenderPass(VkRenderPass& other) = delete;

        VkRenderPass& operator=(const VkRenderPass& other) = delete;

        VkRenderPass(VkRenderPass&& other) noexcept;

        VkRenderPass const& operator=(VkRenderPass&& other) noexcept;

        ~VkRenderPass();

        [[nodiscard]] ::VkRenderPass getRenderPass() const;

        [[nodiscard]] std::vector<VkAttachmentDescription> getAttachments() const;
    };
}
