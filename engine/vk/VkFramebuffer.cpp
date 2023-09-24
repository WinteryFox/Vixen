#include "VkFramebuffer.h"

namespace Vixen::Vk {
    VkFramebuffer::VkFramebuffer(
            const std::shared_ptr<Device> &device,
            const VkRenderPass &renderPass,
            uint32_t width,
            uint32_t height
    ) : device(device),
        framebuffer(VK_NULL_HANDLE) {
        const auto &attachments = renderPass.getAttachments();
        //images.reserve(attachments.size());
        //imageViews.reserve(attachments.size());

        for (size_t i = 0; i < attachments.size(); i++) {
            const auto &attachment = attachments[i];

            images.emplace_back(
                    device,
                    width,
                    height,
                    attachment.format,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
            );

            imageViews.emplace_back(
                    std::make_unique<VkImageView>(
                            images[i],
                            VK_IMAGE_ASPECT_DEPTH_BIT
                    )
            );
        }

        std::vector<::VkImageView> views{imageViews.size()};
        std::transform(imageViews.begin(), imageViews.end(), views.data(), [](const auto &imageView) {
            return imageView->getImageView();
        });
        VkFramebufferCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = renderPass.getRenderPass(),
                .attachmentCount = static_cast<uint32_t>(views.size()),
                .pAttachments = views.data(),
                .width = width,
                .height = height,
                .layers = 0
        };

        checkVulkanResult(
                vkCreateFramebuffer(
                        device->getDevice(),
                        &info,
                        nullptr,
                        &framebuffer
                ),
                "Failed to create framebuffer"
        );
    }

    VkFramebuffer::VkFramebuffer(
            const std::shared_ptr<Device> &device,
            const VkRenderPass &renderPass,
            uint32_t width,
            uint32_t height,
            std::vector<::VkImageView> imageViews
    ) {
        VkFramebufferCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = renderPass.getRenderPass(),
                .attachmentCount = static_cast<uint32_t>(imageViews.size()),
                .pAttachments = imageViews.data(),
                .width = width,
                .height = height,
                .layers = 1
        };

        checkVulkanResult(
                vkCreateFramebuffer(
                        device->getDevice(),
                        &info,
                        nullptr,
                        &framebuffer
                ),
                "Failed to create framebuffer"
        );
    }

    VkFramebuffer::VkFramebuffer(VkFramebuffer &&o) noexcept
            : device(std::move(o.device)),
              images(std::exchange(o.images, {})),
              imageViews(std::exchange(o.imageViews, {})),
              framebuffer(std::exchange(o.framebuffer, nullptr)) {}

    VkFramebuffer::~VkFramebuffer() {
        vkDestroyFramebuffer(device->getDevice(), framebuffer, nullptr);
    }

    ::VkFramebuffer VkFramebuffer::getFramebuffer() const {
        return framebuffer;
    }
}
