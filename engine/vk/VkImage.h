#pragma once

#include <FreeImage.h>
#include "Device.h"
#include "VkBuffer.h"

namespace Vixen::Vk {
    class VkImage {
        const std::shared_ptr<Device> device;

        VmaAllocation allocation;

        uint32_t width;

        uint32_t height;

        VkImageLayout layout;

        VkImageUsageFlags usageFlags;

        ::VkImage image;

        VkFormat format;

    public:
        VkImage(
                const std::shared_ptr<Device> &device,
                uint32_t width,
                uint32_t height,
                VkSampleCountFlagBits samples,
                VkFormat format,
                VkImageTiling tiling,
                VkImageUsageFlags usageFlags
        );

        VkImage(const VkImage &) = delete;

        VkImage(VkImage &&o) noexcept;

        ~VkImage();

        static VkImage from(const std::shared_ptr<Device> &device, const std::string &path);

        void transition(VkImageLayout newLayout);

        void copyFrom(const VkBuffer &buffer);

        [[nodiscard]] VkFormat getFormat() const;

        [[nodiscard]] VkImageUsageFlags getUsageFlags() const;

        [[nodiscard]] ::VkImage getImage() const;

        [[nodiscard]] const std::shared_ptr<Device> &getDevice() const;
    };
}
