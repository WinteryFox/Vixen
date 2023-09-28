#pragma once

#include <memory>
#include "Device.h"
#include "VkFence.h"

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

        template<typename F>
        bool acquireImage(const F &lambda) {
            auto result = imageReadyFences.waitFirst<VkResult>(
                    std::numeric_limits<uint64_t>::max(),
                    true,
                    [this, lambda](const auto &fence) constexpr {
                        uint32_t imageIndex;

                        auto result = vkAcquireNextImageKHR(
                                device->getDevice(),
                                swapchain,
                                std::numeric_limits<uint64_t>::max(),
                                VK_NULL_HANDLE,
                                fence,
                                &imageIndex
                        );

                        lambda(imageIndex, fence);

                        return result;
                    });

            if (result == VK_SUCCESS)
                return false;
            else if (result == VK_SUBOPTIMAL_KHR)
                return true;

            checkVulkanResult(result, "Failed to acquire swapchain image index");
            return true;
        }

        [[nodiscard]] const VkSurfaceFormatKHR &getFormat() const;

        [[nodiscard]] const VkExtent2D &getExtent() const;

        [[nodiscard]] uint32_t getImageCount() const;

        [[nodiscard]] const std::vector<::VkImageView> &getImageViews() const;

        [[nodiscard]] VkSwapchainKHR getSwapchain() const;

    private:
        uint32_t imageCount;

        std::shared_ptr<Device> device;

        VkSwapchainKHR swapchain;

        std::vector<::VkImage> images;

        std::vector<::VkImageView> imageViews;

        std::vector<::VkImage> depthImages;

        std::vector<::VkImageView> depthImageViews;

        VkSurfaceFormatKHR format{};

        VkExtent2D extent{};

        VkFence imageReadyFences;

        std::vector<VkSemaphore> imageAvailableSemaphores;

        static VkSurfaceFormatKHR determineSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available);

        static VkPresentModeKHR determinePresentMode(const std::vector<VkPresentModeKHR> &available);
    };
}
