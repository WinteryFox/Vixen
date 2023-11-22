#pragma once

#include "Device.h"
#include "VkDescriptorPool.h"
#include "VkBuffer.h"

namespace Vixen::Vk {
    class VkDescriptorSet {
        ::VkDescriptorSet set;

        std::shared_ptr<Device> device;

        std::shared_ptr<VkDescriptorPool> pool;

    public:
        VkDescriptorSet(
            const std::shared_ptr<Device>& device,
            const std::shared_ptr<VkDescriptorPool>& pool,
            const VkDescriptorSetLayout& layout
        );

        VkDescriptorSet(VkDescriptorSet& other) = delete;

        VkDescriptorSet& operator=(const VkDescriptorSet& other) = delete;

        VkDescriptorSet(VkDescriptorSet&& fp) noexcept;

        VkDescriptorSet const& operator=(VkDescriptorSet&& fp) noexcept;

        ~VkDescriptorSet();

        void updateUniformBuffer(
            uint32_t binding,
            const VkBuffer &buffer
        ) const;

        [[nodiscard]] ::VkDescriptorSet getSet() const;
    };
}
