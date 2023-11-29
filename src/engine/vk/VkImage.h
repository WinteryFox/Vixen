#pragma once

#include <FreeImagePlus.h>
#include "Device.h"
#include "VkBuffer.h"

namespace Vixen::Vk {
    class VkImage {
        std::shared_ptr<Device> device;

        VmaAllocation allocation;

        uint32_t width;

        uint32_t height;

        VkImageLayout layout;

        VkImageUsageFlags usageFlags;

        ::VkImage image;

        VkFormat format;

    public:
        VkImage(
            const std::shared_ptr<Device>& device,
            uint32_t width,
            uint32_t height,
            VkSampleCountFlagBits samples,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usageFlags
        );

        VkImage(VkImage& other) = delete;

        VkImage& operator=(const VkImage& other) = delete;

        VkImage(VkImage&& other) noexcept;

        VkImage const& operator=(VkImage&& other) noexcept;

        ~VkImage();

        static VkImage from(const std::shared_ptr<Device>& device, const std::string& path);

        void transition(VkImageLayout newLayout);

        void copyFrom(const VkBuffer& buffer);

        [[nodiscard]] VkFormat getFormat() const;

        [[nodiscard]] VkImageLayout getLayout() const;

        [[nodiscard]] VkImageUsageFlags getUsageFlags() const;

        [[nodiscard]] ::VkImage getImage() const;

        [[nodiscard]] const std::shared_ptr<Device>& getDevice() const;
    };
}
