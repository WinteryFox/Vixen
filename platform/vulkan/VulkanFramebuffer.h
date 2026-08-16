#pragma once

#include "core/Framebuffer.h"

namespace Vixen {
    struct VulkanFramebuffer final : Framebuffer {
        VkImage resolveImage;

        VkImageView resolveImageView;

        VkImageSubresourceRange resolveSubresourceRange;

        bool swapchainAcquired = false;
    };
}
