#pragma once

#include "Device.h"
#include "VkRenderPass.h"
#include "VkImageView.h"

namespace Vixen::Vk {
    class VkFramebuffer {
        std::shared_ptr<Device> device;

        std::vector<std::shared_ptr<VkImage>> images;

        std::vector<std::unique_ptr<VkImageView>> imageViews;

        ::VkFramebuffer framebuffer;

    public:
        VkFramebuffer(
                const std::shared_ptr<Device> &device,
                const VkRenderPass &renderPass,
                uint32_t width,
                uint32_t height
        );

        VkFramebuffer(
                const std::shared_ptr<Device> &device,
                const VkRenderPass &renderPass,
                uint32_t width,
                uint32_t height,
                std::vector<::VkImageView> imageViews
        );

        VkFramebuffer(const VkFramebuffer &) = delete;

        VkFramebuffer(VkFramebuffer &&o) noexcept;

        VkFramebuffer &operator=(const VkFramebuffer &) = delete;

        ~VkFramebuffer();

        [[nodiscard]] ::VkFramebuffer getFramebuffer() const;
    };
}
