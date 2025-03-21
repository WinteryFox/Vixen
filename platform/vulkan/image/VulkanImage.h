#pragma once

#include <vk_mem_alloc.h>

#include "VulkanRenderingDeviceDriver.h"
#include "core/image/Image.h"

namespace Vixen {
    struct VulkanImage final : Image {
        VkImage image;

        VkImageView imageView;

        VmaAllocation allocation;
    };
}
