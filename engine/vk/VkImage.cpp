#include "VkImage.h"

namespace Vixen::Vk {
    VkImage::VkImage(
            const std::shared_ptr<Device> &device,
            uint32_t width,
            uint32_t height,
            VkSampleCountFlagBits samples,
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
                .samples = samples,
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

    VkImage VkImage::from(const std::shared_ptr<Device> &device, const std::string &path) {
        FreeImage_Initialise();

        const auto &format = FreeImage_GetFileType(path.c_str(), 0);
        if (format == FIF_UNKNOWN)
            error("Failed to determine image format, possibly unsupported format?");

        const auto &bitmap = FreeImage_Load(format, path.c_str(), 0);
        if (!bitmap)
            error("Failed to load image from file \"{}\"", path);

        const auto &converted = FreeImage_ConvertTo32Bits(bitmap);

        uint32_t width = FreeImage_GetWidth(converted);
        uint32_t height = FreeImage_GetHeight(converted);
        auto pixels = FreeImage_GetBits(converted);

        VkDeviceSize size = width * height * sizeof(uint32_t);

        auto staging = VkBuffer(device, Buffer::Usage::INDEX, size);
        staging.write(pixels, size, 0);

        FreeImage_Unload(converted);
        FreeImage_Unload(bitmap);

        FreeImage_DeInitialise();

        auto image = VkImage(
                device,
                width,
                height,
                VK_SAMPLE_COUNT_1_BIT,
                VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_SAMPLED_BIT
        );

        image.transition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        image.copyFrom(staging);
        image.transition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        return image;
    }

    void VkImage::transition(VkImageLayout newLayout) {
        throw std::runtime_error("Not implemented");
    }

    void VkImage::copyFrom(VkBuffer const &buffer) {
        throw std::runtime_error("Not implemented");
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
