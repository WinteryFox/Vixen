#include "VkImage.h"

namespace Vixen::Vk {
    VkImage::VkImage(
        const std::shared_ptr<Device>& device,
        const uint32_t width,
        const uint32_t height,
        const VkSampleCountFlagBits samples,
        const VkFormat format,
        const VkImageTiling tiling,
        const VkImageUsageFlags usageFlags,
        const uint8_t mipLevels
    ) : device(device),
        allocation(VK_NULL_HANDLE),
        width(width),
        height(height),
        layout(VK_IMAGE_LAYOUT_UNDEFINED),
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
        : device(std::exchange(other.device, nullptr)),
          allocation(std::exchange(other.allocation, VK_NULL_HANDLE)),
          width(other.width),
          height(other.height),
          layout(other.layout),
          usageFlags(other.usageFlags),
          image(std::exchange(other.image, VK_NULL_HANDLE)),
          format(other.format),
          mipLevels(other.mipLevels) {}

    VkImage& VkImage::operator=(VkImage&& other) noexcept {
        std::swap(device, other.device);
        std::swap(allocation, other.allocation);
        std::swap(width, other.width);
        std::swap(height, other.height);
        std::swap(layout, other.layout);
        std::swap(usageFlags, other.usageFlags);
        std::swap(image, other.image);
        std::swap(format, other.format);
        std::swap(mipLevels, other.mipLevels);

        return *this;
    }

    VkImage::~VkImage() {
        if (device != nullptr)
            vmaDestroyImage(device->getAllocator(), image, allocation);
    }

    void VkImage::upload(const VkBuffer& data) {
        const auto& cmd = device->getTransferCommandPool()
                                ->allocate(CommandBufferLevel::PRIMARY);

        cmd.begin(CommandBufferUsage::SINGLE);

        cmd.transitionImage(*this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        cmd.copyBufferToImage(data, *this);

        cmd.transitionImage(*this, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        cmd.end();
        cmd.submit(device->getTransferQueue(), {}, {}, {});
    }

    VkImage VkImage::from(const std::shared_ptr<Device>& device, const std::string& path) {
        FreeImage_Initialise();

        const auto& format = FreeImage_GetFileType(path.c_str(), static_cast<int>(path.length()));
        if (format == FIF_UNKNOWN)
            error("Failed to determine image format, possibly unsupported format?");

        const auto& bitmap = FreeImage_Load(format, path.c_str(), 0);
        if (!bitmap)
            error("Failed to load image from file \"{}\"", path);

        return from(device, bitmap);
    }

    VkImage VkImage::from(const std::shared_ptr<Device>& device, const std::string& format, std::byte* data,
                          const uint32_t size) {
        // TODO: Add some way to detect the format
        const auto& memory = FreeImage_OpenMemory(reinterpret_cast<BYTE*>(data), size);
        if (!memory)
            error("Failed to open image from memory");

        const auto& bitmap = FreeImage_LoadFromMemory(FreeImage_GetFIFFromFormat(format.c_str()), memory, 0);
        if (!bitmap)
            throw std::runtime_error("Failed to load image from memory");

        return from(device, bitmap);
    }

    VkImage VkImage::from(const std::shared_ptr<Device>& device, const VkBuffer& buffer, const uint32_t width,
                          const uint32_t height, const VkFormat format) {
        auto image = VkImage(
            device,
            width,
            height,
            VK_SAMPLE_COUNT_1_BIT,
            format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1
        );

        // TODO
        // image.transition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        image.upload(buffer);
        // image.transition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        return image;
    }

    uint32_t VkImage::getWidth() const { return width; }

    uint32_t VkImage::getHeight() const { return height; }

    ::VkImage VkImage::getImage() const {
        return image;
    }

    const std::shared_ptr<Device>& VkImage::getDevice() const {
        return device;
    }

    uint8_t VkImage::getMipLevels() const {
        return mipLevels;
    }

    VkImage VkImage::from(const std::shared_ptr<Device>& device, FIBITMAP* bitmap) {
        const auto& converted = FreeImage_ConvertTo32Bits(bitmap);
        if (!converted)
            error("Failed to convert image to 32 bits");

        const auto& width = FreeImage_GetWidth(converted);
        const auto& height = FreeImage_GetHeight(converted);
        const auto& bitsPerPixel = FreeImage_GetBPP(converted);
        const auto& pixels = FreeImage_GetBits(converted);

        const VkDeviceSize size = width * height * (bitsPerPixel / 8);

        auto staging = VkBuffer(device, Buffer::Usage::UNIFORM | Buffer::Usage::TRANSFER_SRC, size);
        staging.write(reinterpret_cast<std::byte*>(pixels), size, 0);

        FreeImage_Unload(converted);
        FreeImage_Unload(bitmap);

        FreeImage_DeInitialise();

        VkFormat f;
        // TODO: This will need a better implementation to detect the exact format later
        switch (bitsPerPixel) {
        case 24:
            f = VK_FORMAT_B8G8R8_SRGB;
            break;
        case 32:
            f = VK_FORMAT_B8G8R8A8_SRGB;
            break;
        default:
            throw std::runtime_error("Failed to determine format");
        }

        return from(device, staging, width, height, f);
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
