#pragma once

#include <vma/vk_mem_alloc.h>

#include "VulkanRenderingDevice.h"
#include "core/image/Image.h"

namespace Vixen {
    struct VulkanImage final : Image {
        VkImage image;

        VkImageView imageView;

        VmaAllocation allocation;
    };
}
