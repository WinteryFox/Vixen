#include "VkImageView.h"

#include "Device.h"

namespace Vixen::Vk {
    VkImageView::VkImageView(const std::shared_ptr<Device> &device, const std::shared_ptr<VkImage> &image,
                             const VkImageAspectFlags aspectFlags)
        : device(device),
          image(image),
          imageView(VK_NULL_HANDLE),
          sampler(VK_NULL_HANDLE) {
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

        const VkSamplerCreateInfo samplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 1,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0f,
            .maxLod = static_cast<float>(image->getMipLevels()),
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE
        };
        checkVulkanResult(
            vkCreateSampler(
                image->getDevice()->getDevice(),
                &samplerCreateInfo,
                nullptr,
                &sampler
            ),
            "Failed to create sampler"
        );
    }

    VkImageView::VkImageView(VkImageView &&o) noexcept
        : device(std::exchange(o.device, nullptr)),
          image(std::exchange(o.image, nullptr)),
          imageView(std::exchange(o.imageView, nullptr)),
          sampler(std::exchange(o.sampler, nullptr)) {}

    VkImageView &VkImageView::operator=(VkImageView &&o) noexcept {
        std::swap(device, o.device);
        std::swap(image, o.image);
        std::swap(imageView, o.imageView);
        std::swap(sampler, o.sampler);

        return *this;
    }

    VkImageView::~VkImageView() {
        if (device == nullptr)
            return;

        vkDestroySampler(
            device->getDevice(),
            sampler,
            nullptr
        );

        vkDestroyImageView(
            device->getDevice(),
            imageView,
            nullptr
        );
    }

    ::VkImageView VkImageView::getImageView() const { return imageView; }

    const VkSampler &VkImageView::getSampler() const { return sampler; }

    std::shared_ptr<VkImage> VkImageView::getImage() const { return image; }
}
