#pragma once

#include <vector>
#include <volk.h>

#include "core/Swapchain.h"

namespace Vixen {
    struct VulkanImage;

    struct VulkanSwapchain : Swapchain {
        VkSwapchainKHR swapchain;
        VulkanSurface *surface;
        VkFormat format;
        VkColorSpaceKHR colorSpace;
        std::vector<VkImage> resolveImages;
        std::vector<VkImageView> resolveImageViews;
        std::vector<VulkanImage *> colorTargets;
        std::vector<VulkanImage *> depthTargets;
        uint32_t imageIndex;
    };
}
