#pragma once

#include "Device.h"
#include "VkDescriptorPoolFixed.h"

namespace Vixen::Vk {
    class VkDescriptorPoolExpanding {
    public:
        static constexpr uint32_t maxSets = 4092;

        static constexpr float scaler = 1.5F;

        struct PoolSizeRatio {
            VkDescriptorType type;

            float ratio;
        };

    private:
        std::shared_ptr<Device> device;

        uint32_t setsPerPool;

        std::vector<PoolSizeRatio> ratios;

        std::vector<std::shared_ptr<VkDescriptorPoolFixed>> readyPools;

        std::vector<std::shared_ptr<VkDescriptorPoolFixed>> fullPools;

        [[nodiscard]] std::shared_ptr<VkDescriptorPoolFixed> createPool(
            uint32_t setCount,
            std::span<PoolSizeRatio> poolSizeRatios
        ) const;

        std::shared_ptr<VkDescriptorPoolFixed> getPool();

    public:
        explicit VkDescriptorPoolExpanding(
            const std::shared_ptr<Device>& device,
            uint32_t maxSets,
            std::span<PoolSizeRatio> poolSizeRatios
        );

        VkDescriptorPoolExpanding(const VkDescriptorPoolExpanding&) = delete;

        VkDescriptorPoolExpanding& operator=(const VkDescriptorPoolExpanding&) = delete;

        VkDescriptorPoolExpanding(VkDescriptorPoolExpanding&& fp) noexcept;

        VkDescriptorPoolExpanding& operator=(VkDescriptorPoolExpanding&& fp) noexcept;

        ~VkDescriptorPoolExpanding() = default;

        void reset();

        std::unique_ptr<VkDescriptorSet> allocate(const VkDescriptorSetLayout& layout);
    };
}
