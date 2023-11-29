#pragma once

#include "VkImage.h"

namespace Vixen::Vk {
    class VkImageView {
        std::shared_ptr<VkImage> image;

        ::VkImageView imageView;

    public:
        VkImageView(const std::shared_ptr<VkImage> &image, VkImageAspectFlags aspectFlags);

        VkImageView(const VkImageView &) = delete;

//        VkImageView(VkImageView &&o) noexcept;

        VkImageView &operator=(const VkImageView &) = delete;

        ~VkImageView();

        [[nodiscard]] ::VkImageView getImageView() const;

        [[nodiscard]] std::shared_ptr<VkImage> getImage() const;
    };
}
