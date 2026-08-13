#pragma once

#include "core/Framebuffer.h"

namespace Vixen {
    struct VulkanFramebuffer final : Framebuffer {
        VkImage colorImage;
        VkImageView colorImageView;
        VkImageSubresourceRange subresourceRange;
        bool swapchainAcquired;
    };
}
