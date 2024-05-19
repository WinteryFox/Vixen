#pragma once

#include <stb_image.h>
#include "Disposable.h"
#include "VkBuffer.h"

namespace Vixen::Vk {
    class VkImage final : public Disposable {
        friend class VkCommandBuffer;

        std::shared_ptr<Device> device;

        VmaAllocation allocation;

        uint32_t width;

        uint32_t height;

        VkImageUsageFlags usageFlags;

        ::VkImage image;

        VkFormat format;

        uint8_t mipLevels;

    public:
        VkImage(
            const std::shared_ptr<Device> &device,
            uint32_t width,
            uint32_t height,
            VkSampleCountFlagBits samples,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usageFlags,
            uint8_t mipLevels,
            VkImageLayout initialLayout
        );

        VkImage(
            const std::shared_ptr<Device> &device,
            ::VkImage image,
            uint32_t width,
            uint32_t height,
            VkFormat format,
            VkImageUsageFlags usageFlags,
            uint8_t mipLevels
        );

        VkImage(VkImage &other) = delete;

        VkImage &operator=(const VkImage &other) = delete;

        VkImage(VkImage &&other) noexcept;

        VkImage &operator=(VkImage &&other) noexcept;

        ~VkImage() override = default;

        void dispose() const override;

        void upload(const VkBuffer &data);

        static VkImage from(const std::shared_ptr<Device> &device, const std::string &path);

        static VkImage from(const std::shared_ptr<Device> &device, const std::byte *data, uint32_t size);

        static VkImage from(const std::shared_ptr<Device> &device, const VkBuffer &data, uint32_t width,
                            uint32_t height, VkFormat format);

        [[nodiscard]] uint32_t getWidth() const;

        [[nodiscard]] uint32_t getHeight() const;

        [[nodiscard]] VkFormat getFormat() const;

        [[nodiscard]] VkImageUsageFlags getUsageFlags() const;

        [[nodiscard]] ::VkImage getImage() const;

        [[nodiscard]] const std::shared_ptr<Device> &getDevice() const;

        [[nodiscard]] uint8_t getMipLevels() const;
    };
}
