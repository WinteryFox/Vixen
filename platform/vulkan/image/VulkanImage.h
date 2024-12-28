#pragma once

#include <vma/vk_mem_alloc.h>

#include "VulkanRenderingDevice.h"
#include "core/image/Image.h"

namespace Vixen {
    struct VulkanImage final : Image {
        VulkanImage(
            const ImageFormat &format,
            const ImageView &view,
            VkImage image,
            VkImageView image_view,
            VmaAllocation allocation
        ) : Image(format, view),
            image(image),
            imageView(image_view),
            allocation(allocation) {
        }

        VkImage image;

        VkImageView imageView;

        VmaAllocation allocation;
    };
}
