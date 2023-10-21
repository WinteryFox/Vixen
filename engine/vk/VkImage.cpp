#include "VkImage.h"

namespace Vixen::Vk {
    VkImage::VkImage(
            const std::shared_ptr<Device> &device,
            uint32_t width,
            uint32_t height,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usageFlags
    ) : device(device),
        allocation(VK_NULL_HANDLE),
        image(VK_NULL_HANDLE),
        width(width),
        height(height),
        format(format),
        usageFlags(usageFlags),
        layout(VK_IMAGE_LAYOUT_UNDEFINED) {
        VkImageCreateInfo imageCreateInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = format,
                .extent{
                        .width = width,
                        .height = height,
                        .depth = 1
                },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = tiling,
                .usage = usageFlags,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = layout
        };

        VmaAllocationCreateInfo allocationCreateInfo = {
                .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO,
                .priority = 1.0f
        };

        checkVulkanResult(
                vmaCreateImage(
                        device->getAllocator(),
                        &imageCreateInfo,
                        &allocationCreateInfo,
                        &image,
                        &allocation,
                        nullptr
                ),
                "Failed to create image"
        );
    }

    VkImage::VkImage(VkImage &&o) noexcept:
            allocation(std::exchange(o.allocation, VK_NULL_HANDLE)),
            width(o.width),
            height(o.height),
            layout(o.layout),
            usageFlags(o.usageFlags),
            device(o.device),
            image(std::exchange(o.image, VK_NULL_HANDLE)),
            format(o.format) {}

    VkImage::~VkImage() {
        vmaDestroyImage(device->getAllocator(), image, allocation);
    }

    ::VkImage VkImage::getImage() const {
        return image;
    }

    const std::shared_ptr<Device> &VkImage::getDevice() const {
        return device;
    }

    VkFormat VkImage::getFormat() const {
        return format;
    }

    VkImageUsageFlags VkImage::getUsageFlags() const {
        return usageFlags;
    }
}
