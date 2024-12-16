#pragma once

#include <memory>
#include <vma/vk_mem_alloc.h>
#include <stb_image.h>

#include "core/Samples.h"

namespace Vixen {
    class VulkanBuffer;
    class VulkanDevice;

    class VulkanImage {
        friend class VulkanCommandBuffer;

        std::shared_ptr<VulkanDevice> device;

        VmaAllocation allocation;

        uint32_t width;

        uint32_t height;

        VkImageUsageFlags usageFlags;

        VkImage image;

        VkFormat format;

        uint8_t mipLevels;

        Samples sampleCount;

    public:
        VulkanImage(
            const std::shared_ptr<VulkanDevice> &device,
            uint32_t width,
            uint32_t height,
            Samples sampleCount,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usageFlags,
            uint8_t mipLevels
        );

        VulkanImage(VulkanImage &other) = delete;

        VulkanImage &operator=(const VulkanImage &other) = delete;

        VulkanImage(VulkanImage &&other) noexcept;

        VulkanImage &operator=(VulkanImage &&other) noexcept;

        ~VulkanImage();

        void upload(const VulkanBuffer &data);

        static VulkanImage from(const std::shared_ptr<VulkanDevice> &device, const std::string &path);

        static VulkanImage from(const std::shared_ptr<VulkanDevice> &device, const std::byte *data, uint32_t size);

        static VulkanImage from(const std::shared_ptr<VulkanDevice> &device, const VulkanBuffer &data, uint32_t width,
                            uint32_t height, VkFormat format);

        [[nodiscard]] uint32_t getWidth() const;

        [[nodiscard]] uint32_t getHeight() const;

        [[nodiscard]] VkFormat getFormat() const;

        [[nodiscard]] VkImageUsageFlags getUsageFlags() const;

        [[nodiscard]] ::VkImage getImage() const;

        [[nodiscard]] const std::shared_ptr<VulkanDevice> &getDevice() const;

        [[nodiscard]] uint8_t getMipLevels() const;

        [[nodiscard]] Samples getSampleCount() const;
    };
}
