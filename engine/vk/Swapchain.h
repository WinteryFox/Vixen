#pragma once

#include <memory>
#include "Device.h"

namespace Vixen::Vk {
    class Swapchain {
    public:
        enum class FramesInFlight {
            SINGLE_BUFFER = 0,
            DOUBLE_BUFFER = 1,
            TRIPLE_BUFFER = 2
        };

        Swapchain(const std::shared_ptr<Device> &device, FramesInFlight framesInFlight);

        ~Swapchain();

    private:
        uint32_t imageCount;

        std::shared_ptr<Device> device;

        VkSwapchainKHR swapchain = VK_NULL_HANDLE;

        std::vector<VkImage> images;

        std::vector<VkImageView> imageViews;

        VkSurfaceFormatKHR format{};

        VkExtent2D extent{};

        static VkSurfaceFormatKHR determineSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available);

        static VkPresentModeKHR determinePresentMode(const std::vector<VkPresentModeKHR> &available);
    };
}
