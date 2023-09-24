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
        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = width;
        imageCreateInfo.extent.height = height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.format = format;
        imageCreateInfo.tiling = tiling;
        imageCreateInfo.initialLayout = layout;
        imageCreateInfo.usage = usageFlags;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocationCreateInfo = {};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        checkVulkanResult(
                vmaCreateImage(
                        device->getAllocator()->getAllocator(),
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
        vmaDestroyImage(device->getAllocator()->getAllocator(), image, allocation);
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
}
