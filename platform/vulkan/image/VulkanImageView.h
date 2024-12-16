#pragma once

#include "VulkanImage.h"

namespace Vixen {
    class VulkanImageView {
        std::shared_ptr<VulkanDevice> device;

        std::shared_ptr<VulkanImage> image;

        ::VkImageView imageView;

        VkSampler sampler;

    public:
        VulkanImageView(const std::shared_ptr<VulkanDevice> &device, const std::shared_ptr<VulkanImage> &image,
                    VkImageAspectFlags aspectFlags);

        VulkanImageView(const VulkanImageView &) = delete;

        VulkanImageView &operator=(const VulkanImageView &) = delete;

        VulkanImageView(VulkanImageView &&o) noexcept;

        VulkanImageView &operator=(VulkanImageView &&o) noexcept;

        ~VulkanImageView();

        [[nodiscard]] ::VkImageView getImageView() const;

        [[nodiscard]] const VkSampler &getSampler() const;

        [[nodiscard]] std::shared_ptr<VulkanImage> getImage() const;
    };
}
