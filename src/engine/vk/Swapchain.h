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
        State acquireImage(uint64_t timeout, const F &lambda) {
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

            lambda(currentFrame, imageIndex, imageAvailableSemaphore);

            currentFrame = (currentFrame + 1) % imageCount;

            switch (result) {
                using
                enum State;

                case VK_SUCCESS:
                    return OK;
                case VK_SUBOPTIMAL_KHR:
                    spdlog::warn("Suboptimal swapchain state");
                    return SUBOPTIMAL;
                default:
                    checkVulkanResult(result, "Failed to acquire swapchain image");
                    return OUT_OF_DATE;
            }
        }

        [[nodiscard]] const VkSurfaceFormatKHR &getFormat() const;

        [[nodiscard]] const VkExtent2D &getExtent() const;

        [[nodiscard]] uint32_t getImageCount() const;

        [[nodiscard]] const std::vector<::VkImageView> &getImageViews() const;

        void present(uint32_t imageIndex, const std::vector<::VkSemaphore> &waitSemaphores);

        void invalidate();

    private:
        std::shared_ptr<Device> device;

        uint32_t currentFrame;

        uint32_t imageCount;

        VkSurfaceFormatKHR format;

        VkSwapchainKHR swapchain;

        std::vector<::VkImage> images;

        std::vector<::VkImageView> imageViews;

        VkExtent2D extent{};

        std::vector<VkSemaphore> imageAvailableSemaphores;

        static VkSurfaceFormatKHR determineSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available);

        static VkPresentModeKHR determinePresentMode(const std::vector<VkPresentModeKHR> &available);

        void create();

        void destroy();
    };
}
