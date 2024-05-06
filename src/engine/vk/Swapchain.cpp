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

        for (const auto &format: available)
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return format;

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

    const VkSurfaceFormatKHR &Swapchain::getColorFormat() const {
        return format;
    }

    VkFormat Swapchain::getDepthFormat() const {
        return depthImages[0]->getFormat();
    }

    const VkExtent2D &Swapchain::getExtent() const {
        return extent;
    }

    uint32_t Swapchain::getImageCount() const {
        return imageCount;
    }

    const std::vector<std::shared_ptr<VkImage> > &Swapchain::getImages() const { return images; }

    const std::vector<VkImageView> &Swapchain::getImageViews() const { return imageViews; }

    const std::vector<std::shared_ptr<VkImage> > &Swapchain::getDepthImages() const { return depthImages; }

    const std::vector<VkImageView> &Swapchain::getDepthImageViews() const { return depthImageViews; }

    uint32_t Swapchain::getCurrentFrame() const { return currentFrame; }

    void Swapchain::present(uint32_t imageIndex, const std::vector<::VkSemaphore> &waitSemaphores) {
        const auto &commandBuffer = device->getTransferCommandPool()->allocate(CommandBufferLevel::Primary);
        commandBuffer.begin(CommandBufferUsage::Once);
        commandBuffer.transitionImage(*images[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0,
                                      images[imageIndex]->getMipLevels());
        commandBuffer.record([this, &imageIndex](const auto &cmd) {
            VkImageMemoryBarrier barrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_NONE,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = internalImages[imageIndex],
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier
            );

            const VkImageCopy region{
                .srcSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
                .srcOffset = {
                    .x = 0,
                    .y = 0,
                    .z = 0
                },
                .dstSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
                .dstOffset = {
                    .x = 0,
                    .y = 0,
                    .z = 0
                },
                .extent = {
                    .width = extent.width,
                    .height = extent.height,
                    .depth = 1
                }
            };

            vkCmdCopyImage(
                cmd,
                images[imageIndex]->getImage(),
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                internalImages[imageIndex],
                barrier.newLayout,
                1,
                &region
            );

            barrier.oldLayout = barrier.newLayout;
            barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.srcAccessMask = VK_ACCESS_NONE;
            barrier.dstAccessMask = VK_ACCESS_NONE;

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier
            );
        });
        commandBuffer.transitionImage(*images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0,
                                      images[imageIndex]->getMipLevels());
        commandBuffer.end();
        commandBuffer.submit(device->getTransferQueue(), {}, {}, {});

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
        const auto &surface = device->getSurface();
        const auto &capabilities = device->getGpu().getSurfaceCapabilities(device->getSurface());

        const auto &presentMode = determinePresentMode(device->getGpu().getPresentModes(surface));
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
            .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
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

        vkGetSwapchainImagesKHR(device->getDevice(), swapchain, &imageCount, nullptr);
        internalImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device->getDevice(), swapchain, &imageCount, internalImages.data());

        images.reserve(imageCount);
        imageViews.reserve(imageCount);
        depthImages.reserve(imageCount);
        depthImageViews.reserve(imageCount);
        imageAvailableSemaphores.reserve(imageCount);
        for (auto i = 0; i < imageCount; i++) {
            images.emplace_back(
                std::make_shared<VkImage>(
                    device,
                    extent.width,
                    extent.height,
                    VK_SAMPLE_COUNT_1_BIT,
                    format.format,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                    1,
                    VK_IMAGE_LAYOUT_UNDEFINED
                )
            );
            imageViews.emplace_back(
                images[i],
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
                    1,
                    VK_IMAGE_LAYOUT_UNDEFINED
                )
            );
            depthImageViews.emplace_back(
                depthImages[i],
                VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
            );

            imageAvailableSemaphores.emplace_back(device);
        }

        const auto &cmd = device->getTransferCommandPool()->allocate(CommandBufferLevel::Primary);
        cmd.begin(CommandBufferUsage::Once);
        for (const auto &image: images)
            cmd.transitionImage(*image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, 1);
        cmd.end();
        cmd.submit(device->getPresentQueue(), {}, {}, {});
    }

    void Swapchain::destroy() {
        depthImageViews.clear();
        depthImages.clear();
        internalImages.clear();
        imageViews.clear();
        images.clear();
        imageAvailableSemaphores.clear();
        vkDestroySwapchainKHR(device->getDevice(), swapchain, nullptr);
    }
}
