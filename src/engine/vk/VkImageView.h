#pragma once

#include "VkImage.h"

namespace Vixen::Vk {
    class VkImageView {
        std::shared_ptr<Device> device;

        std::shared_ptr<VkImage> image;

        ::VkImageView imageView;

        VkSampler sampler;

    public:
        VkImageView(const std::shared_ptr<Device> &device, const std::shared_ptr<VkImage> &image,
                    VkImageAspectFlags aspectFlags);

        VkImageView(const VkImageView &) = delete;

        VkImageView &operator=(const VkImageView &) = delete;

        VkImageView(VkImageView &&o) noexcept;

        VkImageView &operator=(VkImageView &&o) noexcept;

        ~VkImageView();

        [[nodiscard]] ::VkImageView getImageView() const;

        [[nodiscard]] const VkSampler &getSampler() const;

        [[nodiscard]] std::shared_ptr<VkImage> getImage() const;
    };
}
