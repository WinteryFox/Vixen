#pragma once

#include <memory>
#include "Device.h"
#include "VkFence.h"
#include "VkSemaphore.h"

namespace Vixen::Vk {
    class Swapchain {
    public:
        enum class FramesInFlight {
            SINGLE_BUFFER = 1,
            DOUBLE_BUFFER = 2,
            TRIPLE_BUFFER = 3
        };

        Swapchain(const std::shared_ptr<Device> &device, FramesInFlight framesInFlight);

        ~Swapchain();

        /**
         * Waits for and then acquires the next available swapchain image.
         * @param lambda The lambda to execute as soon as an image becomes available.
         * @param timeout The amount of time in microseconds to wait on an image before timing out.
         * @return Returns true when the swapchain is out-of-date, indicating the need to recreate the swapchain,
         * otherwise false.
         */
        template<typename F>
        bool acquireImage(const F &lambda, uint64_t timeout) {
            auto result = inFlightFences.waitFirst<VkResult>(
                    std::numeric_limits<uint64_t>::max(),
                    true,
                    [this, &lambda, &timeout](const auto &fence, const auto &index) constexpr {
                        auto &semaphore = imageAvailableSemaphores[index];

                        uint32_t imageIndex;
                        auto result = vkAcquireNextImageKHR(
                                device->getDevice(),
                                swapchain,
                                timeout,
                                semaphore.getSemaphore(),
                                VK_NULL_HANDLE,
                                &imageIndex
                        );

                        lambda(imageIndex, semaphore, fence);

                        return result;
                    });

            switch (result) {
                case VK_SUCCESS:
                case VK_SUBOPTIMAL_KHR:
                    return false;
                case VK_ERROR_OUT_OF_DATE_KHR:
                    return true;
                default:
                    break;
            }

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

        VkFence inFlightFences;

        std::vector<VkSemaphore> imageAvailableSemaphores;

        static VkSurfaceFormatKHR determineSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available);

        static VkPresentModeKHR determinePresentMode(const std::vector<VkPresentModeKHR> &available);
    };
}
