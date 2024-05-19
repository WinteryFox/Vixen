#define STB_IMAGE_IMPLEMENTATION

#include "VkImage.h"

#include "Device.h"

namespace Vixen::Vk {
    VkImage::VkImage(
        const std::shared_ptr<Device> &device,
        const uint32_t width,
        const uint32_t height,
        const VkSampleCountFlagBits samples,
        const VkFormat format,
        const VkImageTiling tiling,
        const VkImageUsageFlags usageFlags,
        const uint8_t mipLevels,
        const VkImageLayout initialLayout
    ) : device(device),
        allocation(VK_NULL_HANDLE),
        width(width),
        height(height),
        usageFlags(usageFlags),
        image(VK_NULL_HANDLE),
        format(format),
        mipLevels(mipLevels) {
        const VkImageCreateInfo imageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .extent = {
                .width = width,
                .height = height,
                .depth = 1
            },
            .mipLevels = mipLevels,
            .arrayLayers = 1,
            .samples = samples,
            .tiling = tiling,
            .usage = usageFlags | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = initialLayout
        };

        constexpr VmaAllocationCreateInfo allocationCreateInfo = {
            .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = 0,
            .preferredFlags = 0,
            .memoryTypeBits = 0,
            .pool = nullptr,
            .pUserData = nullptr,
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

    VkImage::VkImage(
        const std::shared_ptr<Device> &device,
        ::VkImage image,
        const uint32_t width,
        const uint32_t height,
        const VkFormat format,
        const VkImageUsageFlags usageFlags,
        const uint8_t mipLevels
    ) : device(device),
        allocation(VK_NULL_HANDLE),
        width(width),
        height(height),
        usageFlags(usageFlags),
        image(image),
        format(format),
        mipLevels(mipLevels) {}

    VkImage::VkImage(VkImage &&other) noexcept
        : device(std::exchange(other.device, nullptr)),
          allocation(std::exchange(other.allocation, VK_NULL_HANDLE)),
          width(other.width),
          height(other.height),
          usageFlags(other.usageFlags),
          image(std::exchange(other.image, VK_NULL_HANDLE)),
          format(other.format),
          mipLevels(other.mipLevels) {}

    VkImage &VkImage::operator=(VkImage &&other) noexcept {
        std::swap(device, other.device);
        std::swap(allocation, other.allocation);
        std::swap(width, other.width);
        std::swap(height, other.height);
        std::swap(usageFlags, other.usageFlags);
        std::swap(image, other.image);
        std::swap(format, other.format);
        std::swap(mipLevels, other.mipLevels);

        return *this;
    }

    void VkImage::dispose() const {
        vmaDestroyImage(device->getAllocator(), image, allocation);
    }

    void VkImage::upload(const VkBuffer &data) {
        const auto &cmd = device->getTransferCommandPool()
                ->allocate(CommandBufferLevel::Primary);

        cmd.begin(CommandBufferUsage::Once);

        cmd.transitionImage(*this, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 1);
        cmd.copyBufferToImage(data, *this);
        cmd.transitionImage(*this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 1);

        cmd.blitImage(*this, *this);
        cmd.transitionImage(*this, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0,
                            mipLevels);

        cmd.end();
        cmd.submit(device->getTransferQueue(), {}, {}, {});
    }

    VkImage VkImage::from(const std::shared_ptr<Device> &device, const std::string &path) {
        stbi_set_flip_vertically_on_load(1);

        int width;
        int height;
        int channels;
        stbi_uc *data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!data)
            throw std::runtime_error("Failed to load image");

        const auto &staging = VkBuffer(
            device,
            BufferUsage::Uniform | BufferUsage::CopySource,
            width * height,
            4
        );
        staging.setData(reinterpret_cast<std::byte *>(data));

        stbi_image_free(data);

        return from(device, staging, width, height, VK_FORMAT_R8G8B8A8_SRGB);
    }

    VkImage VkImage::from(const std::shared_ptr<Device> &device, const std::byte *data, const uint32_t size) {
        stbi_set_flip_vertically_on_load(1);

        int width;
        int height;
        int channels;
        stbi_load_from_memory(reinterpret_cast<stbi_uc const *>(data), static_cast<int>(size), &width, &height,
                              &channels, STBI_rgb_alpha);

        const auto &staging = VkBuffer(
            device,
            BufferUsage::Uniform | BufferUsage::CopySource,
            width * height,
            4
        );
        staging.setData(data);

        return from(device, staging, width, height, VK_FORMAT_R8G8B8A8_SRGB);
    }

    VkImage VkImage::from(const std::shared_ptr<Device> &device, const VkBuffer &data, const uint32_t width,
                          const uint32_t height, const VkFormat format) {
        auto image = VkImage(
            device,
            width,
            height,
            VK_SAMPLE_COUNT_1_BIT,
            format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1),
            VK_IMAGE_LAYOUT_UNDEFINED
        );
        image.upload(data);

        return image;
    }

    uint32_t VkImage::getWidth() const { return width; }

    uint32_t VkImage::getHeight() const { return height; }

    ::VkImage VkImage::getImage() const {
        return image;
    }

    const std::shared_ptr<Device> &VkImage::getDevice() const {
        return device;
    }

    uint8_t VkImage::getMipLevels() const {
        return mipLevels;
    }

    VkFormat VkImage::getFormat() const {
        return format;
    }

    VkImageUsageFlags VkImage::getUsageFlags() const {
        return usageFlags;
    }
}
