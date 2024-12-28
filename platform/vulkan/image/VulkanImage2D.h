#pragma once

#include <memory>
#include <vma/vk_mem_alloc.h>
#include <stb_image.h>

#include "VulkanRenderingDevice.h"
#include "core/image/Image2D.h"

namespace Vixen {
    class VulkanBuffer;

    class VulkanImage2D final : public Image2D {
        [[nodiscard]] inline VkSampleCountFlagBits findClosestSupportedSampleCount(const ImageSamples &samples) const;

        std::shared_ptr<VulkanRenderingDevice> device;

        VmaAllocation allocation;

        VkImage image;

        VkImageView imageView;

    public:
        VulkanImage2D(const std::shared_ptr<VulkanRenderingDevice> &device, const ImageFormat &format,
                      const ImageView &view);

        VulkanImage2D(VulkanImage2D &other) = delete;

        VulkanImage2D &operator=(const VulkanImage2D &other) = delete;

        VulkanImage2D(VulkanImage2D &&other) noexcept;

        VulkanImage2D &operator=(VulkanImage2D &&other) noexcept;

        ~VulkanImage2D() override;

        void upload(const VulkanBuffer &data);

        static VulkanImage2D from(const std::shared_ptr<VulkanRenderingDevice> &device, const std::string &path);

        static VulkanImage2D from(const std::shared_ptr<VulkanRenderingDevice> &device, const std::byte *data,
                                  uint32_t size);

        static VulkanImage2D from(const std::shared_ptr<VulkanRenderingDevice> &device, const VulkanBuffer &data,
                                  uint32_t width,
                                  uint32_t height, VkFormat format);

        [[nodiscard]] VkImage getImage() const;
    };
}
