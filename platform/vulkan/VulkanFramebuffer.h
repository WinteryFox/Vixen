#pragma once

#include "core/Framebuffer.h"

namespace Vixen {
    struct VulkanFramebuffer final : Framebuffer {
        VkImage swapchainImage;
        VkImageView swapchainImageView;
        VkImageSubresourceRange subresourceRange;
        bool swapchainAcquired;
    };
}
