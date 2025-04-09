#pragma once

struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

namespace Vixen {
    struct VulkanImage final : Image {
        VkImage image;

        VkImageView imageView;

        VmaAllocation allocation;
    };
}
