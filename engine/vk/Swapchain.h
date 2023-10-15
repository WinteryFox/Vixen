#pragma once

#include <memory>
#include "Device.h"
#include "VkFence.h"
#include "VkSemaphore.h"

namespace Vixen::Vk {
    class Swapchain {
    public:
        enum class State {
            OK,
            SUBOPTIMAL,
            OUT_OF_DATE
        };

        Swapchain(const std::shared_ptr<Device> &device, uint32_t framesInFlight);

        ~Swapchain();

        /**
         * Waits for and then acquires the next available swapchain image.
         * @param lambda The lambda to execute as soon as an image becomes available.
         * @param timeout The amount of time in microseconds to wait on an image before timing out.
         * @return Returns true when the swapchain is out-of-date, indicating the need to recreate the swapchain,
         * otherwise false.
         */
        template<typename F>
        State acquireImage(const F &lambda, uint64_t timeout) {
            auto result = inFlightFences.wait<State>(
                    currentFrame,
                    std::numeric_limits<uint64_t>::max(),
                    [this, &lambda, &timeout](const auto &fence) constexpr {
                        auto &imageAvailableSemaphore = imageAvailableSemaphores[currentFrame];

                        uint32_t imageIndex;
                        auto result = vkAcquireNextImageKHR(
                                device->getDevice(),
                                swapchain,
                                timeout,
                                imageAvailableSemaphore.getSemaphore(),
                                VK_NULL_HANDLE,
                                &imageIndex
                        );

                        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                            spdlog::debug("Swapchain is outdated");
                            return State::OUT_OF_DATE;
                        }

                        vkResetFences(device->getDevice(), 1, &fence);
                        lambda(currentFrame, imageIndex, imageAvailableSemaphore, fence);

                        switch (result) {
                            case VK_SUCCESS:
                                return State::OK;
                            case VK_SUBOPTIMAL_KHR:
                                spdlog::warn("Suboptimal swapchain state");
                                return State::SUBOPTIMAL;
                            default:
                                checkVulkanResult(result, "Failed to acquire swapchain image");
                                return State::OUT_OF_DATE;
                        }
                    });

            currentFrame = (currentFrame + 1) % imageCount;
            return result;
        }

        [[nodiscard]] const VkSurfaceFormatKHR &getFormat() const;

        [[nodiscard]] const VkExtent2D &getExtent() const;

        [[nodiscard]] uint32_t getImageCount() const;

        [[nodiscard]] const std::vector<::VkImageView> &getImageViews() const;

        [[nodiscard]] VkSwapchainKHR getSwapchain() const;

        void invalidate();

    private:
        uint32_t currentFrame;

        uint32_t imageCount;

        std::shared_ptr<Device> device;

        VkSwapchainKHR swapchain;

        std::vector<::VkImage> images;

        std::vector<::VkImageView> imageViews;

        VkSurfaceFormatKHR format{};

        VkExtent2D extent{};

        VkFence inFlightFences;

        std::vector<VkSemaphore> imageAvailableSemaphores;

        static VkSurfaceFormatKHR determineSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available);

        static VkPresentModeKHR determinePresentMode(const std::vector<VkPresentModeKHR> &available);

        void create();

        void destroy();
    };
}
