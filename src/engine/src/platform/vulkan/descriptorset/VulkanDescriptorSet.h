#pragma once

#include <memory>
#include <Volk/volk.h>

namespace Vixen {
    class VulkanBuffer;
    class VulkanImageView;
    class VulkanDescriptorSetLayout;
    class VulkanDescriptorPoolFixed;
    class VulkanDevice;

    class VulkanDescriptorSet {
        VkDescriptorSet set;

        std::shared_ptr<VulkanDevice> device;

        std::shared_ptr<const VulkanDescriptorPoolFixed> pool;

    public:
        VulkanDescriptorSet(
            const std::shared_ptr<VulkanDevice>& device,
            const std::shared_ptr<const VulkanDescriptorPoolFixed>& pool,
            const VulkanDescriptorSetLayout& layout
        );

        VulkanDescriptorSet(VulkanDescriptorSet& other) = delete;

        VulkanDescriptorSet& operator=(const VulkanDescriptorSet& other) = delete;

        VulkanDescriptorSet(VulkanDescriptorSet&& other) noexcept;

        VulkanDescriptorSet& operator=(VulkanDescriptorSet&& other) noexcept;

        ~VulkanDescriptorSet();

        void writeUniformBuffer(
            uint32_t binding,
            const VulkanBuffer& buffer,
            uint32_t offset,
            uint32_t size
        ) const;

        void writeCombinedImageSampler(
            uint32_t binding,
            const VulkanImageView &view
        ) const;

        [[nodiscard]] VkDescriptorSet getSet() const;
    };
}
