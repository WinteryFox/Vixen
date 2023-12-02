#include "VkImage.h"

namespace Vixen::Vk {
    VkImage::VkImage(
        const std::shared_ptr<Device>& device,
        const uint32_t width,
        const uint32_t height,
        const VkSampleCountFlagBits samples,
        const VkFormat format,
        const VkImageTiling tiling,
        const VkImageUsageFlags usageFlags
    ) : device(device),
        allocation(VK_NULL_HANDLE),
        image(VK_NULL_HANDLE),
        width(width),
        height(height),
        format(format),
        usageFlags(usageFlags),
        layout(VK_IMAGE_LAYOUT_UNDEFINED) {
        const VkImageCreateInfo imageCreateInfo{
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

        constexpr VmaAllocationCreateInfo allocationCreateInfo = {
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

    VkImage::VkImage(VkImage&& other) noexcept
        : device(other.device),
          allocation(std::exchange(other.allocation, VK_NULL_HANDLE)),
          width(other.width),
          height(other.height),
          layout(other.layout),
          usageFlags(other.usageFlags),
          image(std::exchange(other.image, VK_NULL_HANDLE)),
          format(other.format) {}

    VkImage const& VkImage::operator=(VkImage&& other) noexcept {
        std::swap(device, other.device);
        std::swap(allocation, other.allocation);
        std::swap(width, other.width);
        std::swap(height, other.height);
        std::swap(layout, other.layout);
        std::swap(usageFlags, other.usageFlags);
        std::swap(image, other.image);
        std::swap(format, other.format);

        return *this;
    }

    VkImage::~VkImage() {
        vmaDestroyImage(device->getAllocator(), image, allocation);
    }

    VkImage VkImage::from(const std::shared_ptr<Device>& device, const std::string& path) {
        FreeImage_Initialise();

        const auto& format = FreeImage_GetFileType(path.c_str(), static_cast<int>(path.length()));
        if (format == FIF_UNKNOWN)
            error("Failed to determine image format, possibly unsupported format?");

        const auto& bitmap = FreeImage_Load(format, path.c_str(), 0);
        if (!bitmap)
            error("Failed to load image from file \"{}\"", path);

        const auto& converted = FreeImage_ConvertTo32Bits(bitmap);

        const auto& width = FreeImage_GetWidth(converted);
        const auto& height = FreeImage_GetHeight(converted);
        const auto& bitsPerPixel = FreeImage_GetBPP(converted);
        const auto& pixels = FreeImage_GetBits(converted);

        const VkDeviceSize size = width * height * (bitsPerPixel / 8);

        auto staging = VkBuffer(device, Buffer::Usage::UNIFORM | Buffer::Usage::TRANSFER_SRC, size);
        staging.write(reinterpret_cast<char*>(pixels), size, 0);

        FreeImage_Unload(converted);
        FreeImage_Unload(bitmap);

        FreeImage_DeInitialise();

        VkFormat f;
        switch (bitsPerPixel) {
        case 24:
            f = VK_FORMAT_R8G8B8_SRGB;
            break;
        case 32:
            f = VK_FORMAT_R8G8B8A8_SRGB;
            break;
        default:
            throw std::runtime_error("Failed to determine format");
        }

        auto image = VkImage(
            device,
            width,
            height,
            VK_SAMPLE_COUNT_1_BIT,
            f,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        );

        image.transition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        image.copyFrom(staging);
        image.transition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        return image;
    }

    void VkImage::transition(VkImageLayout newLayout) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = layout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // TODO: This looks sus too
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;

        VkPipelineStageFlags source;
        VkPipelineStageFlags destination;

        if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            source = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
            newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            source = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destination = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::runtime_error("Unsupported transition layout");
        }

        device->getTransferCommandPool()
              ->allocateCommandBuffer(VkCommandBuffer::Level::PRIMARY)
              .record(
                  VkCommandBuffer::Usage::SINGLE,
                  [this, &source, &destination, &barrier](auto commandBuffer) {
                      vkCmdPipelineBarrier(
                          commandBuffer,
                          source,
                          destination,
                          0,
                          0,
                          nullptr,
                          0,
                          nullptr,
                          1,
                          &barrier
                      );
                  }
              )
              .submit(device->getTransferQueue(), {}, {}, {});

        layout = newLayout;
    }

    void VkImage::copyFrom(VkBuffer const& buffer) {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        device->getTransferCommandPool()
              ->allocateCommandBuffer(VkCommandBuffer::Level::PRIMARY)
              .record(
                  VkCommandBuffer::Usage::SINGLE,
                  [this, &buffer, &region](const auto& commandBuffer) {
                      vkCmdCopyBufferToImage(commandBuffer, buffer.getBuffer(), image, layout, 1, &region);
                  }
              )
              .submit(device->getTransferQueue(), {}, {}, {});
    }

    ::VkImage VkImage::getImage() const {
        return image;
    }

    const std::shared_ptr<Device>& VkImage::getDevice() const {
        return device;
    }

    VkFormat VkImage::getFormat() const {
        return format;
    }

    VkImageLayout VkImage::getLayout() const {
        return layout;
    }

    VkImageUsageFlags VkImage::getUsageFlags() const {
        return usageFlags;
    }
}
