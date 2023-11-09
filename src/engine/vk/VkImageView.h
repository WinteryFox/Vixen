#pragma once

#include "VkImage.h"

namespace Vixen::Vk {
    class VkImageView {
        const VkImage &image;

        ::VkImageView imageView;

    public:
        VkImageView(const VkImage &image, VkImageAspectFlags aspectFlags);

        VkImageView(const VkImageView &) = delete;

//        VkImageView(VkImageView &&o) noexcept;

        VkImageView &operator=(const VkImageView &) = delete;

        ~VkImageView();

        [[nodiscard]] ::VkImageView getImageView() const;
    };
}
