#pragma once

#include <memory>
#include "Device.h"
#include "VkFence.h"
#include "VkSemaphore.h"

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
                    return false;
                case VK_SUBOPTIMAL_KHR:
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
