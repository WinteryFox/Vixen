#include "Swapchain.h"

namespace Vixen::Vk {
    Swapchain::Swapchain(const std::shared_ptr<Device>& device, uint32_t framesInFlight)
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

    VkSurfaceFormatKHR Swapchain::determineSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available) {
        if (available.empty())
            throw std::runtime_error("Failed to find suitable surface format");

        for (const auto& format : available) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return format;
        }

        return available[0];
    }

    VkPresentModeKHR Swapchain::determinePresentMode(const std::vector<VkPresentModeKHR>& available) {
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

    const VkSurfaceFormatKHR& Swapchain::getColorFormat() const {
        return format;
    }

    VkFormat Swapchain::getDepthFormat() const {
        return depthImages[0]->getFormat();
    }

    const VkExtent2D& Swapchain::getExtent() const {
        return extent;
    }

    uint32_t Swapchain::getImageCount() const {
        return imageCount;
    }

    const std::vector<std::shared_ptr<VkImage>>& Swapchain::getColorImages() const { return colorImages; }

    const std::vector<VkImageView>& Swapchain::getColorImageViews() const { return colorImageViews; }

    const std::vector<std::shared_ptr<VkImage>>& Swapchain::getDepthImages() const { return depthImages; }

    const std::vector<VkImageView>& Swapchain::getDepthImageViews() const { return depthImageViews; }

    void Swapchain::present(uint32_t imageIndex, const std::vector<::VkSemaphore>& waitSemaphores) {
        const VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,

            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
            .pWaitSemaphores = waitSemaphores.data(),

            .swapchainCount = 1,
            .pSwapchains = &swapchain,

            .pImageIndices = &imageIndex,
            .pResults = nullptr
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

        const VkPresentModeKHR presentMode = determinePresentMode(device->getGpu().getPresentModes(surface));
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
            const std::vector indices = {
                device->getGraphicsQueueFamily().index,
                device->getPresentQueueFamily().index
            };

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

        std::vector<::VkImage> swapchainImages;
        vkGetSwapchainImagesKHR(device->getDevice(), swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device->getDevice(), swapchain, &imageCount, swapchainImages.data());

        colorImages.reserve(imageCount);
        colorImageViews.reserve(imageCount);
        depthImages.reserve(imageCount);
        depthImageViews.reserve(imageCount);
        imageAvailableSemaphores.reserve(imageCount);
        for (auto i = 0; i < imageCount; i++) {
            colorImages.emplace_back(
                std::make_shared<VkImage>(
                    device,
                    swapchainImages[i],
                    extent.width,
                    extent.height,
                    format.format,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    1
                )
            );
            colorImageViews.emplace_back(
                colorImages[i],
                VK_IMAGE_ASPECT_COLOR_BIT
            );

            depthImages.push_back(
                std::make_shared<VkImage>(
                    device,
                    extent.width,
                    extent.height,
                    VK_SAMPLE_COUNT_1_BIT,
                    device->getGpu().pickFormat(
                        {
                            VK_FORMAT_D32_SFLOAT_S8_UINT,
                            VK_FORMAT_D24_UNORM_S8_UINT
                        },
                        VK_IMAGE_TILING_OPTIMAL,
                        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
                    ),
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    1
                )
            );
            depthImageViews.emplace_back(
                depthImages[i],
                VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
            );

            imageAvailableSemaphores.emplace_back(device);
        }
    }

    void Swapchain::destroy() {
        depthImageViews.clear();
        depthImages.clear();
        colorImageViews.clear();
        colorImages.clear();
        imageAvailableSemaphores.clear();
        vkDestroySwapchainKHR(device->getDevice(), swapchain, nullptr);
    }
}
