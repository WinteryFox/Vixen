#define STB_IMAGE_IMPLEMENTATION

#include "VulkanImage.h"

#include <Vulkan.h>

#include "device/VulkanDevice.h"
#include "buffer/VulkanBuffer.h"
#include "commandbuffer/CommandBufferLevel.h"
#include "commandbuffer/CommandBufferUsage.h"
#include "commandbuffer/VulkanCommandPool.h"
#include "core/BufferUsage.h"

namespace Vixen {
    VulkanImage::VulkanImage(
        const std::shared_ptr<VulkanDevice> &device,
        const uint32_t width,
        const uint32_t height,
        const Samples sampleCount,
        const VkFormat format,
        const VkImageTiling tiling,
        const VkImageUsageFlags usageFlags,
        const uint8_t mipLevels
    ) : device(device),
        allocation(VK_NULL_HANDLE),
        width(width),
        height(height),
        usageFlags(usageFlags),
        image(VK_NULL_HANDLE),
        format(format),
        mipLevels(mipLevels),
        sampleCount(sampleCount) {
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
            .samples = toVkSampleCountFlagBits(sampleCount),
            .tiling = tiling,
            .usage = usageFlags | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
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

    VulkanImage::VulkanImage(VulkanImage &&other) noexcept
        : device(std::exchange(other.device, nullptr)),
          allocation(std::exchange(other.allocation, VK_NULL_HANDLE)),
          width(other.width),
          height(other.height),
          usageFlags(other.usageFlags),
          image(std::exchange(other.image, VK_NULL_HANDLE)),
          format(other.format),
          mipLevels(other.mipLevels),
          sampleCount(other.sampleCount) {}

    VulkanImage &VulkanImage::operator=(VulkanImage &&other) noexcept {
        std::swap(device, other.device);
        std::swap(allocation, other.allocation);
        std::swap(width, other.width);
        std::swap(height, other.height);
        std::swap(usageFlags, other.usageFlags);
        std::swap(image, other.image);
        std::swap(format, other.format);
        std::swap(mipLevels, other.mipLevels);
        std::swap(sampleCount, other.sampleCount);

        return *this;
    }

    VulkanImage::~VulkanImage() {
        if (device)
            vmaDestroyImage(device->getAllocator(), image, allocation);
    }

    void VulkanImage::upload(const VulkanBuffer &data) {
        const auto &cmd = device->getTransferCommandPool()->allocate(CommandBufferLevel::Primary);
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

    VulkanImage VulkanImage::from(const std::shared_ptr<VulkanDevice> &device, const std::string &path) {
        stbi_set_flip_vertically_on_load(1);

        int width;
        int height;
        int channels;
        stbi_uc *data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!data)
            throw std::runtime_error("Failed to load image");

        const auto &staging = VulkanBuffer(
            device,
            BufferUsage::Uniform | BufferUsage::CopySource,
            width * height,
            4
        );
        staging.setData(std::bit_cast<std::byte *>(data));

        stbi_image_free(data);

        return from(device, staging, width, height, VK_FORMAT_R8G8B8A8_SRGB);
    }

    VulkanImage VulkanImage::from(const std::shared_ptr<VulkanDevice> &device, const std::byte *data,
                                  const uint32_t size) {
        stbi_set_flip_vertically_on_load(1);

        int width;
        int height;
        int channels;
        stbi_load_from_memory(
            std::bit_cast<stbi_uc const *>(data),
            static_cast<int>(size),
            &width,
            &height,
            &channels,
            STBI_rgb_alpha
        );

        // TODO: For some reason this is broken and I have no clue why
        const auto &staging = VulkanBuffer(
            device,
            BufferUsage::Uniform | BufferUsage::CopySource,
            width * height,
            channels
        );
        staging.setData(data);

        return from(device, staging, width, height, VK_FORMAT_R8G8B8A8_SRGB);
    }

    VulkanImage VulkanImage::from(const std::shared_ptr<VulkanDevice> &device, const VulkanBuffer &data,
                                  const uint32_t width,
                                  const uint32_t height, const VkFormat format) {
        auto image = VulkanImage(
            device,
            width,
            height,
            Samples::None,
            format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
            static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1)
        );
        image.upload(data);

        return image;
    }

    uint32_t VulkanImage::getWidth() const { return width; }

    uint32_t VulkanImage::getHeight() const { return height; }

    VkImage VulkanImage::getImage() const { return image; }

    const std::shared_ptr<VulkanDevice> &VulkanImage::getDevice() const { return device; }

    uint8_t VulkanImage::getMipLevels() const { return mipLevels; }

    Samples VulkanImage::getSampleCount() const { return sampleCount; }

    VkFormat VulkanImage::getFormat() const { return format; }

    VkImageUsageFlags VulkanImage::getUsageFlags() const { return usageFlags; }
}
