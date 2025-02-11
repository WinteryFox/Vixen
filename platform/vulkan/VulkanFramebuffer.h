#pragma once

#include "core/Framebuffer.h"
#include "image/VulkanImage.h"

namespace Vixen {
    struct VulkanFramebuffer : Framebuffer {
        VkFramebuffer framebuffer;
        VkImage swapchainImage;
        VkImageSubresourceRange subresourceRange;
        bool swapchainAcquired;
    };
}
