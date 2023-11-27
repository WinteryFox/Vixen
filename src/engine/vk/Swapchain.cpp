#include "Swapchain.h"

namespace Vixen::Vk {
    Swapchain::Swapchain(const std::shared_ptr<Device> &device, uint32_t framesInFlight)
            : device(device),
              currentFrame(0),
              imageCount(framesInFlight),
              format(determineSurfaceFormat(device->getGpu().getSurfaceFormats(device->getSurface()))),
              swapchain(VK_NULL_HANDLE) {
        create();
    }

    Swapchain::~Swapchain() {
        destroy();
    }

    VkSurfaceFormatKHR Swapchain::determineSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available) {
        if (available.empty())
            throw std::runtime_error("Failed to find suitable surface format");

        for (const auto &format: available) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return format;
        }

        return available[0];
    }

    VkPresentModeKHR Swapchain::determinePresentMode(const std::vector<VkPresentModeKHR> &available) {
        if (available.empty())
            throw std::runtime_error("Failed to find suitable present mode");

        if (std::ranges::contains(available, VK_PRESENT_MODE_MAILBOX_KHR))
            return VK_PRESENT_MODE_MAILBOX_KHR;
        if (std::ranges::contains(available, VK_PRESENT_MODE_FIFO_KHR))
            return VK_PRESENT_MODE_FIFO_KHR;
        if (std::ranges::contains(available, VK_PRESENT_MODE_FIFO_RELAXED_KHR))
            return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        if (std::ranges::contains(available, VK_PRESENT_MODE_IMMEDIATE_KHR))
            return VK_PRESENT_MODE_IMMEDIATE_KHR;

        return available[0];
    }

    const VkSurfaceFormatKHR &Swapchain::getFormat() const {
        return format;
    }

    const VkExtent2D &Swapchain::getExtent() const {
        return extent;
    }

    uint32_t Swapchain::getImageCount() const {
        return imageCount;
    }

    const std::vector<::VkImageView> &Swapchain::getImageViews() const {
        return imageViews;
    }

    void Swapchain::present(uint32_t imageIndex, const std::vector<::VkSemaphore> &waitSemaphores) {
        VkPresentInfoKHR presentInfo{
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,

                .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
                .pWaitSemaphores = waitSemaphores.data(),

                .swapchainCount = 1,
                .pSwapchains = &swapchain,

                .pImageIndices = &imageIndex,
                .pResults = nullptr,
        };

        vkQueuePresentKHR(device->getPresentQueue(), &presentInfo);
    }

    void Swapchain::invalidate() {
        device->waitIdle();
        destroy();
        create();
    }

    void Swapchain::create() {
        const auto surface = device->getSurface();
        const auto capabilities = device->getGpu().getSurfaceCapabilities(device->getSurface());

        VkPresentModeKHR presentMode = determinePresentMode(device->getGpu().getPresentModes(surface));
        extent = capabilities.currentExtent;

        spdlog::trace("Selected present mode {} and image format {} and color space {}",
                      string_VkPresentModeKHR(presentMode),
                      string_VkFormat(format.format), string_VkColorSpaceKHR(format.colorSpace));

        VkSwapchainCreateInfoKHR info{
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .surface = device->getSurface(),
                .minImageCount = imageCount,
                .imageFormat = format.format,
                .imageColorSpace = format.colorSpace,
                .imageExtent = extent,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .preTransform = capabilities.currentTransform,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode = presentMode,
                .clipped = VK_TRUE,
                .oldSwapchain = VK_NULL_HANDLE,
        };

        if (device->getGraphicsQueueFamily().index != device->getPresentQueueFamily().index) {
            std::vector<uint32_t> indices = {device->getGraphicsQueueFamily().index,
                                             device->getPresentQueueFamily().index};

            info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            info.queueFamilyIndexCount = indices.size();
            info.pQueueFamilyIndices = indices.data();
        } else {
            info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            info.queueFamilyIndexCount = 0;
            info.pQueueFamilyIndices = nullptr;
        }

        checkVulkanResult(
                vkCreateSwapchainKHR(device->getDevice(), &info, nullptr, &swapchain),
                "Failed to create swapchain"
        );

        vkGetSwapchainImagesKHR(device->getDevice(), swapchain, &imageCount, nullptr);
        images.resize(imageCount);
        vkGetSwapchainImagesKHR(device->getDevice(), swapchain, &imageCount, images.data());

        imageViews.resize(images.size());
        imageAvailableSemaphores.reserve(images.size());
        for (auto i = 0; i < images.size(); i++) {
            VkImageViewCreateInfo imageViewCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = images[i],
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = format.format,
                    .components = {
                            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                    },
                    .subresourceRange = {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = 0,
                            .levelCount = 1,
                            .baseArrayLayer = 0,
                            .layerCount = 1
                    }
            };

            checkVulkanResult(
                    vkCreateImageView(device->getDevice(), &imageViewCreateInfo, nullptr, &imageViews[i]),
                    "Failed to create image view"
            );

            imageAvailableSemaphores.emplace_back(device);
        }
    }

    void Swapchain::destroy() {
        for (auto &imageView: imageViews)
            vkDestroyImageView(device->getDevice(), imageView, nullptr);
        vkDestroySwapchainKHR(device->getDevice(), swapchain, nullptr);
    }
}
