#pragma once

#include <vector>
#include <volk.h>

namespace Vixen {
    struct VulkanSwapchain {
        VkSwapchainKHR swapchain;
        VkSurfaceKHR surface;
        VkFormat format;
        VkColorSpaceKHR colorSpace;
        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
        uint32_t imageIndex;
    };
}
