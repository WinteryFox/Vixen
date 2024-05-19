#include "VulkanSwapchain.h"

#include "VulkanDevice.h"
#include "commandbuffer/CommandBufferLevel.h"
#include "commandbuffer/CommandBufferUsage.h"
#include "commandbuffer/VulkanCommandPool.h"
#include "core/CommandPoolUsage.h"
#include "image/VulkanImageView.h"

namespace Vixen {
    VulkanSwapchain::VulkanSwapchain(const std::shared_ptr<VulkanDevice> &device, uint32_t framesInFlight)
        : device(device),
          currentFrame(0),
          imageCount(framesInFlight),
          format(determineSurfaceFormat(device->getGpu().getSurfaceFormats(device->getSurface()))),
          swapchain(VK_NULL_HANDLE) {
        create();
    }

    VulkanSwapchain::~VulkanSwapchain() {
        destroy();
    }

    VkSurfaceFormatKHR VulkanSwapchain::determineSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available) {
        if (available.empty())
            throw std::runtime_error("Failed to find suitable surface format");

        for (const auto &format: available)
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return format;

        return available[0];
    }

    VkPresentModeKHR VulkanSwapchain::determinePresentMode(const std::vector<VkPresentModeKHR> &available) {
        if (available.empty())
            throw std::runtime_error("Failed to find suitable present mode");

        if (std::ranges::find(available, VK_PRESENT_MODE_MAILBOX_KHR) != available.end())
            return VK_PRESENT_MODE_MAILBOX_KHR;
        if (std::ranges::find(available, VK_PRESENT_MODE_FIFO_KHR) != available.end())
            return VK_PRESENT_MODE_FIFO_KHR;
        if (std::ranges::find(available, VK_PRESENT_MODE_FIFO_RELAXED_KHR) != available.end())
            return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        if (std::ranges::find(available, VK_PRESENT_MODE_IMMEDIATE_KHR) != available.end())
            return VK_PRESENT_MODE_IMMEDIATE_KHR;

        return available[0];
    }

    const VkSurfaceFormatKHR &VulkanSwapchain::getColorFormat() const {
        return format;
    }

    VkFormat VulkanSwapchain::getDepthFormat() const {
        return frames[0].depthTarget->getFormat();
    }

    const VkExtent2D &VulkanSwapchain::getExtent() const {
        return extent;
    }

    uint32_t VulkanSwapchain::getImageCount() const {
        return imageCount;
    }

    const std::vector<FrameData> &VulkanSwapchain::getFrames() const { return frames; }

    uint32_t VulkanSwapchain::getCurrentFrame() const { return currentFrame; }

    void VulkanSwapchain::present(const std::vector<::VkSemaphore> &waitSemaphores) {
        const auto &commandBuffer = device->getTransferCommandPool()->allocate(CommandBufferLevel::Primary);
        commandBuffer.begin(CommandBufferUsage::Once);
        commandBuffer.transitionImage(*frames[currentFrame].colorTarget, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 1);
        commandBuffer.record([this](const auto &cmd) {
            VkImageMemoryBarrier barrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_NONE,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = internalImages[currentFrame],
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
                frames[currentFrame].colorTarget->getImage(),
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                internalImages[currentFrame],
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
        commandBuffer.transitionImage(*frames[currentFrame].colorTarget, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, 1);
        commandBuffer.end();
        commandBuffer.submit(device->getTransferQueue(), {}, {}, {});

        const VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,

            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
            .pWaitSemaphores = waitSemaphores.data(),

            .swapchainCount = 1,
            .pSwapchains = &swapchain,

            .pImageIndices = &currentFrame,
            .pResults = nullptr
        };

        vkQueuePresentKHR(device->getPresentQueue(), &presentInfo);

        currentFrame = (currentFrame + 1) % imageCount;
    }

    void VulkanSwapchain::invalidate() {
        device->waitIdle();
        destroy();
        create();
    }

    void VulkanSwapchain::create() {
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

        frames.reserve(imageCount);
        for (auto i = 0; i < imageCount; i++) {
            const auto &image = std::make_shared<VulkanImage>(
                device,
                extent.width,
                extent.height,
                VK_SAMPLE_COUNT_1_BIT,
                format.format,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                1,
                VK_IMAGE_LAYOUT_UNDEFINED
            );
            const auto &commandPool = device->allocateCommandPool(CommandPoolUsage::Graphics, true);

            const auto &depthTarget = std::make_shared<VulkanImage>(
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
            );

            frames.push_back(
                {
                    device,
                    image,
                    std::make_shared<VulkanImageView>(
                        device,
                        image,
                        VK_IMAGE_ASPECT_COLOR_BIT
                    ),
                    depthTarget,
                    std::make_shared<VulkanImageView>(
                        device,
                        depthTarget,
                        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
                    ),
                    commandPool,
                    commandPool->allocate(CommandBufferLevel::Primary),
                    VulkanSemaphore(device),
                    VulkanSemaphore(device)
                }
            );
        }

        const auto &cmd = device->getTransferCommandPool()->allocate(CommandBufferLevel::Primary);
        cmd.begin(CommandBufferUsage::Once);
        for (const auto &frame: frames)
            cmd.transitionImage(*frame.colorTarget, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0,
                                1);
        cmd.end();
        cmd.submit(device->getPresentQueue(), {}, {}, {});
    }

    void VulkanSwapchain::destroy() {
        internalImages.clear();
        frames.clear();
        vkDestroySwapchainKHR(device->getDevice(), swapchain, nullptr);
    }
}
