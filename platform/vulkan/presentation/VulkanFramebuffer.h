#pragma once

#include <volk.h>

#include "core/rendering/Framebuffer.h"

namespace Vixen {
    struct VulkanFramebuffer final : Framebuffer {
        VkImage resolveImage;

        VkImageView resolveImageView;

        VkImageSubresourceRange resolveSubresourceRange;

        bool swapchainAcquired = false;
    };
}
