#pragma once

#include <memory>
#include "Device.h"

namespace Vixen::Engine {
    class VkSwapchain {
    public:
        enum class FramesInFlight {
            SINGLE_BUFFER = 0,
            DOUBLE_BUFFER = 1,
            TRIPLE_BUFFER = 2
        };

        VkSwapchain(const std::shared_ptr<Device> &device, FramesInFlight framesInFlight);

        ~VkSwapchain();

    private:
        uint32_t imageCount;

        std::shared_ptr<Device> device;

        VkSwapchainKHR swapchain;

        std::vector<VkImage> images;

        std::vector<VkImageView> imageViews;

        VkSurfaceFormatKHR format;

        VkExtent2D extent;

        static VkSurfaceFormatKHR determineSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available);

        static VkPresentModeKHR determinePresentMode(const std::vector<VkPresentModeKHR> &available);
    };
}
