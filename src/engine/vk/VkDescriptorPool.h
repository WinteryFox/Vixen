#pragma once

#include "VkDescriptorSetLayout.h"

namespace Vixen::Vk {
    class VkDescriptorPool {
        std::shared_ptr<Device> device;

        ::VkDescriptorPool pool;

    public:
        VkDescriptorPool(
            const std::shared_ptr<Device>& device,
            const std::vector<VkDescriptorPoolSize>& sizes,
            uint32_t maxSets
        );

        VkDescriptorPool(VkDescriptorPool& other) = delete;

        VkDescriptorPool& operator=(const VkDescriptorPool& other) = delete;

        VkDescriptorPool(VkDescriptorPool&& fp) noexcept;

        VkDescriptorPool const& operator=(VkDescriptorPool&& fp) noexcept;

        ~VkDescriptorPool();

        [[nodiscard]] ::VkDescriptorSet allocate(const VkDescriptorSetLayout &layout) const;

        [[nodiscard]] std::shared_ptr<Device> getDevice() const;

        [[nodiscard]] ::VkDescriptorPool getPool() const;
    };
}
