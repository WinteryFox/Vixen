#pragma once

#include <memory>
#include "Device.h"
#include "VkImage.h"
#include "VkImageView.h"
#include "VkSemaphore.h"

namespace Vixen::Vk {
    struct FrameData {
        std::shared_ptr<Device> device;

        std::shared_ptr<VkImage> colorTarget;

        std::shared_ptr<VkImageView> colorImageView;

        std::shared_ptr<VkImage> depthTarget;

        std::shared_ptr<VkImageView> depthImageView;

        std::shared_ptr<VkCommandPool> commandPool;

        VkCommandBuffer commandBuffer;

        DeletionQueue deletionQueue;

        VkSemaphore imageAvailableSemaphore;

        VkSemaphore renderFinishedSemaphore;
    };

    class Swapchain {
        std::shared_ptr<Device> device;

        uint32_t currentFrame;

        uint32_t imageCount;

        VkSurfaceFormatKHR format;

        VkSwapchainKHR swapchain;

        std::vector<::VkImage> internalImages;

        std::vector<FrameData> frames;

        VkExtent2D extent{};

        static VkSurfaceFormatKHR determineSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available);

        static VkPresentModeKHR determinePresentMode(const std::vector<VkPresentModeKHR> &available);

        void create();

        void destroy();

    public:
        enum class State {
            Ok,
            Suboptimal,
            OutOfDate
        };

        Swapchain(const std::shared_ptr<Device> &device, uint32_t framesInFlight);

        Swapchain(const Swapchain &) = delete;

        Swapchain &operator=(const Swapchain &) = delete;

        // TODO: Implement these
        Swapchain(Swapchain &&other) noexcept = delete;

        Swapchain &operator=(Swapchain &&other) noexcept = delete;

        ~Swapchain();

        /**
         * Waits for and then acquires the next available swapchain image.
         * @param lambda The lambda to execute as soon as an image becomes available.
         * @param timeout The amount of time in microseconds to wait on an image before timing out.
         * @return Returns true when the swapchain is out-of-date, indicating the need to recreate the swapchain,
         * otherwise false.
         */
        template<typename F>
        State acquireImage(const uint64_t timeout, const F &lambda) {
            const auto &frame = frames[currentFrame];

            uint32_t imageIndex;
            const auto &result = vkAcquireNextImageKHR(
                device->getDevice(),
                swapchain,
                timeout,
                frame.imageAvailableSemaphore.getSemaphore(),
                VK_NULL_HANDLE,
                &imageIndex
            );

            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                spdlog::debug("Swapchain is outdated");
                return State::OutOfDate;
            }

            lambda(frame);

            switch (result) {
                    using enum State;

                case VK_SUCCESS:
                    return Ok;
                case VK_SUBOPTIMAL_KHR:
                    spdlog::warn("Suboptimal swapchain state");
                    return Suboptimal;
                default:
                    checkVulkanResult(result, "Failed to acquire swapchain image");
                    return OutOfDate;
            }
        }

        [[nodiscard]] const VkSurfaceFormatKHR &getColorFormat() const;

        [[nodiscard]] VkFormat getDepthFormat() const;

        [[nodiscard]] const VkExtent2D &getExtent() const;

        [[nodiscard]] uint32_t getImageCount() const;

        [[nodiscard]] const std::vector<FrameData> &getFrames() const;

        [[nodiscard]] uint32_t getCurrentFrame() const;

        void present(const std::vector<::VkSemaphore> &waitSemaphores);

        void invalidate();
    };
}
