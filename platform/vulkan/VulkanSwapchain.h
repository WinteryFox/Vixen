#pragma once

#include <vector>
#include <volk.h>

#include "core/Swapchain.h"

namespace Vixen {
    struct VulkanSwapchain : Swapchain {
        VkSwapchainKHR swapchain;
        VkSurfaceKHR surface;
        VkFormat format;
        VkColorSpaceKHR colorSpace;
        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
        uint32_t imageIndex;
    };
}
