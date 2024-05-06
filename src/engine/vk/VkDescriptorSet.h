#pragma once

#include "Device.h"
#include "VkBuffer.h"
#include "VkImageView.h"

namespace Vixen::Vk {
    class VkDescriptorSetLayout;
    class VkDescriptorPoolFixed;

    class VkDescriptorSet {
        ::VkDescriptorSet set;

        std::shared_ptr<Device> device;

        std::shared_ptr<const VkDescriptorPoolFixed> pool;

    public:
        VkDescriptorSet(
            const std::shared_ptr<Device>& device,
            const std::shared_ptr<const VkDescriptorPoolFixed>& pool,
            const VkDescriptorSetLayout& layout
        );

        VkDescriptorSet(VkDescriptorSet& other) = delete;

        VkDescriptorSet& operator=(const VkDescriptorSet& other) = delete;

        VkDescriptorSet(VkDescriptorSet&& other) noexcept;

        VkDescriptorSet& operator=(VkDescriptorSet&& other) noexcept;

        ~VkDescriptorSet();

        void writeUniformBuffer(
            uint32_t binding,
            const VkBuffer& buffer,
            uint32_t offset,
            uint32_t size
        ) const;

        void writeCombinedImageSampler(
            uint32_t binding,
            const VkImageView &view
        ) const;

        [[nodiscard]] ::VkDescriptorSet getSet() const;
    };
}
