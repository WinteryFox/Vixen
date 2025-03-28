#pragma once

#include <vector>
#include <volk.h>

#include "VulkanFramebuffer.h"
#include "VulkanSurface.h"
#include "command/VulkanCommandQueue.h"
#include "core/Swapchain.h"
#include "image/VulkanImage.h"

namespace Vixen {
    struct VulkanSwapchain final : Swapchain {
        VkSwapchainKHR swapchain;
        VulkanSurface *surface;
        VkFormat format;
        VkColorSpaceKHR colorSpace;
        std::vector<VkImage> resolveImages;
        std::vector<VkImageView> resolveImageViews;
        std::vector<VulkanFramebuffer*> framebuffers;
        std::vector<VulkanCommandQueue*> acquiredCommandQueues;
        std::vector<uint32_t> acquiredCommandQueueSemaphores;
        std::vector<VulkanImage *> colorTargets;
        std::vector<VulkanImage *> depthTargets;
        uint32_t imageIndex;
    };
}
