#pragma once

#include <span>

#include "VulkanDescriptorPoolFixed.h"

namespace Vixen {
    class VulkanDescriptorSetLayout;
    class VulkanDescriptorSet;
    class VulkanDevice;
    class VulkanDescriptorPoolFixed;

    class VulkanDescriptorPoolExpanding {
    public:
        static constexpr uint32_t maxSets = 4092;

        static constexpr float scaler = 1.5F;

        struct PoolSizeRatio {
            VkDescriptorType type;

            float ratio;
        };

    private:
        std::shared_ptr<VulkanDevice> device;

        uint32_t setsPerPool;

        std::vector<PoolSizeRatio> ratios;

        std::vector<std::shared_ptr<VulkanDescriptorPoolFixed>> readyPools;

        std::vector<std::shared_ptr<VulkanDescriptorPoolFixed>> fullPools;

        [[nodiscard]] std::shared_ptr<VulkanDescriptorPoolFixed> createPool(
            uint32_t setCount,
            std::span<PoolSizeRatio> poolSizeRatios
        ) const;

        std::shared_ptr<VulkanDescriptorPoolFixed> getPool();

    public:
        explicit VulkanDescriptorPoolExpanding(
            const std::shared_ptr<VulkanDevice>& device,
            uint32_t maxSets,
            std::span<PoolSizeRatio> poolSizeRatios
        );

        VulkanDescriptorPoolExpanding(const VulkanDescriptorPoolExpanding&) = delete;

        VulkanDescriptorPoolExpanding& operator=(const VulkanDescriptorPoolExpanding&) = delete;

        VulkanDescriptorPoolExpanding(VulkanDescriptorPoolExpanding&& fp) noexcept;

        VulkanDescriptorPoolExpanding& operator=(VulkanDescriptorPoolExpanding&& fp) noexcept;

        ~VulkanDescriptorPoolExpanding() = default;

        void reset();

        std::shared_ptr<VulkanDescriptorSet> allocate(const VulkanDescriptorSetLayout& layout);
    };
}
