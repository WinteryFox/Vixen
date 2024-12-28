#define STB_IMAGE_IMPLEMENTATION

#include "VulkanImage2D.h"

#include <Vulkan.h>

#include "buffer/VulkanBuffer.h"

namespace Vixen {
    VkSampleCountFlagBits VulkanImage2D::findClosestSupportedSampleCount(const ImageSamples &samples) const {
        const auto limits = device->getPhysicalDevice().properties.limits;

        const VkSampleCountFlags flags = limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts;
        if (flags & toVkSampleCountFlagBits(samples))
            return toVkSampleCountFlagBits(samples);

        VkSampleCountFlagBits sampleCount = toVkSampleCountFlagBits(samples);
        while (sampleCount > VK_SAMPLE_COUNT_1_BIT) {
            if (flags & sampleCount) {
                return sampleCount;
            }

            sampleCount = static_cast<VkSampleCountFlagBits>(sampleCount >> 1);
        }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    VulkanImage2D::VulkanImage2D(
        const std::shared_ptr<VulkanRenderingDevice> &device,
        const ImageFormat &format,
        const ImageView &view
    ) : Image2D(format, view),
        allocation(VK_NULL_HANDLE),
        image(VK_NULL_HANDLE),
        imageView(VK_NULL_HANDLE) {
        VkImageCreateInfo imageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = toVkDataFormat[format.format],
            .extent = {
                .width = format.width,
                .height = format.height,
                .depth = format.depth
            },
            .mipLevels = format.mipmapCount,
            .arrayLayers = format.layerCount,
            .samples = findClosestSupportedSampleCount(format.samples),
            .tiling = format.usage & CpuRead ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL,
            .usage = 0,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        if (format.usage & Sampling)
            imageCreateInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        if (format.usage & ImageUsage::Storage)
            imageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        if (format.usage & ColorAttachment)
            imageCreateInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (format.usage & DepthStencilAttachment)
            imageCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        if (format.usage & InputAttachment)
            imageCreateInfo.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        if (format.usage & Update)
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (format.usage & ImageUsage::CopySource)
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if (format.usage & ImageUsage::CopyDestination)
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo allocationCreateInfo{
            .flags = static_cast<VmaAllocationCreateFlags>(
                format.usage & CpuRead
                    ? VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
                    : 0
            ),
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = 0,
            .preferredFlags = 0,
            .memoryTypeBits = 0,
            .pool = nullptr,
            .pUserData = nullptr,
            .priority = 0.0f
        };

        if (format.usage & Transient) {
            uint32_t memoryTypeIndex = 0;
            VmaAllocationCreateInfo lazyMemoryRequirements = allocationCreateInfo;
            lazyMemoryRequirements.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
            if (VkResult result = vmaFindMemoryTypeIndex(device->getAllocator(), UINT32_MAX, &lazyMemoryRequirements,
                                                         &memoryTypeIndex); result == VK_SUCCESS) {
                allocationCreateInfo = lazyMemoryRequirements;
                imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
                imageCreateInfo.usage &= (VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                          | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                          VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
            } else {
                allocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            }
        } else {
            allocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }

        // TODO: Handle small allocations

        ASSERT_THROW(
            vmaCreateImage(
                device->getAllocator(),
                &imageCreateInfo,
                &allocationCreateInfo,
                &image,
                &allocation,
                nullptr
            ) == VK_SUCCESS,
            CantCreateError,
            "Failed to create image"
        );

        VkImageViewCreateInfo imageViewInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = imageCreateInfo.format,
            .components = {
                .r = static_cast<VkComponentSwizzle>(view.swizzleRed),
                .g = static_cast<VkComponentSwizzle>(view.swizzleGreen),
                .b = static_cast<VkComponentSwizzle>(view.swizzleBlue),
                .a = static_cast<VkComponentSwizzle>(view.swizzleAlpha)
            },
            .subresourceRange = {
                .aspectMask = static_cast<VkImageAspectFlags>(format.usage & DepthStencilAttachment
                                                                  ? VK_IMAGE_ASPECT_DEPTH_BIT
                                                                  : VK_IMAGE_ASPECT_COLOR_BIT),
                .baseMipLevel = 0,
                .levelCount = imageCreateInfo.mipLevels,
                .baseArrayLayer = 0,
                .layerCount = imageCreateInfo.arrayLayers
            }
        };

        ASSERT_THROW(vkCreateImageView(device->getDevice(), &imageViewInfo, nullptr, &imageView) == VK_SUCCESS,
                     CantCreateError, "Call to vkCreateImageView failed.");
    }

    VulkanImage2D::VulkanImage2D(VulkanImage2D &&other) noexcept
        : Image2D(format, view),
          device(std::exchange(other.device, nullptr)),
          allocation(std::exchange(other.allocation, VK_NULL_HANDLE)),
          image(std::exchange(other.image, VK_NULL_HANDLE)),
          imageView(std::exchange(other.imageView, VK_NULL_HANDLE)) {
    }

    VulkanImage2D &VulkanImage2D::operator=(VulkanImage2D &&other) noexcept {
        std::swap(device, other.device);
        std::swap(allocation, other.allocation);
        std::swap(image, other.image);
        std::swap(imageView, other.imageView);

        return *this;
    }

    VulkanImage2D::~VulkanImage2D() {
        if (device)
            vmaDestroyImage(device->getAllocator(), image, allocation);
    }

    // void VulkanImage2D::upload(const VulkanBuffer &data) {
    //     const auto &cmd = device->getTransferCommandPool()->allocate(CommandBufferLevel::Primary);
    //     cmd.begin(CommandBufferUsage::Once);
    //
    //     cmd.transitionImage(*this, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 1);
    //     cmd.copyBufferToImage(data, *this);
    //     cmd.transitionImage(*this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 1);
    //
    //     cmd.blitImage(*this, *this);
    //     cmd.transitionImage(*this, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0,
    //                         mipLevels);
    //
    //     cmd.end();
    //     cmd.submit(device->getTransferQueue(), {}, {}, {});
    // }
    //
    // VulkanImage2D VulkanImage2D::from(const std::shared_ptr<VulkanRenderingDevice> &device, const std::string &path) {
    //     stbi_set_flip_vertically_on_load(1);
    //
    //     int width;
    //     int height;
    //     int channels;
    //     stbi_uc *data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    //     if (!data)
    //         throw std::runtime_error("Failed to load image");
    //
    //     const auto &staging = VulkanBuffer(
    //         device,
    //         BufferUsage::Uniform | BufferUsage::CopySource,
    //         width * height,
    //         4
    //     );
    //     staging.setData(std::bit_cast<std::byte *>(data));
    //
    //     stbi_image_free(data);
    //
    //     return from(device, staging, width, height, VK_FORMAT_R8G8B8A8_SRGB);
    // }
    //
    // VulkanImage2D VulkanImage2D::from(const std::shared_ptr<VulkanRenderingDevice> &device, const std::byte *data,
    //                                   const uint32_t size) {
    //     stbi_set_flip_vertically_on_load(1);
    //
    //     int width;
    //     int height;
    //     int channels;
    //     stbi_load_from_memory(
    //         std::bit_cast<stbi_uc const *>(data),
    //         static_cast<int>(size),
    //         &width,
    //         &height,
    //         &channels,
    //         STBI_rgb_alpha
    //     );
    //
    //     // TODO: For some reason this is broken and I have no clue why
    //     const auto &staging = VulkanBuffer(
    //         device,
    //         BufferUsage::Uniform | BufferUsage::CopySource,
    //         width * height,
    //         channels
    //     );
    //     staging.setData(data);
    //
    //     return from(device, staging, width, height, VK_FORMAT_R8G8B8A8_SRGB);
    // }
    //
    // VulkanImage2D VulkanImage2D::from(const std::shared_ptr<VulkanRenderingDevice> &device, const VulkanBuffer &data,
    //                                   const uint32_t width,
    //                                   const uint32_t height, const VkFormat format) {
    //     auto image = VulkanImage2D(
    //         device,
    //         width,
    //         height,
    //         Samples::None,
    //         format,
    //         VK_IMAGE_TILING_OPTIMAL,
    //         VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
    //         VK_IMAGE_USAGE_TRANSFER_DST_BIT |
    //         VK_IMAGE_USAGE_SAMPLED_BIT,
    //         static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1)
    //     );
    //     image.upload(data);
    //
    //     return image;
    // }

    VkImage VulkanImage2D::getImage() const { return image; }
}
