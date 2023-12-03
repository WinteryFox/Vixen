#include "VkImageView.h"

namespace Vixen::Vk {
    VkImageView::VkImageView(const std::shared_ptr<VkImage>& image, const VkImageAspectFlags aspectFlags)
        : image(image),
          imageView(VK_NULL_HANDLE) {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = image->getImage();
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = image->getFormat();
        imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = image->getMipLevels();
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        checkVulkanResult(
            vkCreateImageView(image->getDevice()->getDevice(), &imageViewCreateInfo, nullptr, &imageView),
            "Failed to create image view"
        );
    }

    //    VkImageView::VkImageView(VkImageView &&o) noexcept
    //            : image(o.image),
    //              imageView(std::exchange(o.imageView, VK_NULL_HANDLE)) {}

    VkImageView::~VkImageView() {
        vkDestroyImageView(
            image->getDevice()->getDevice(),
            imageView,
            nullptr
        );
    }

    ::VkImageView VkImageView::getImageView() const {
        return imageView;
    }

    std::shared_ptr<VkImage> VkImageView::getImage() const {
        return image;
    }
}
