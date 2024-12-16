#include "VulkanDescriptorPoolExpanding.h"

#include <spdlog/spdlog.h>

#include "core/exception/OutOfPoolMemoryException.h"

namespace Vixen {
    class OutOfPoolMemoryException;

    std::shared_ptr<VulkanDescriptorPoolFixed> VulkanDescriptorPoolExpanding::createPool(
        const uint32_t setCount,
        const std::span<PoolSizeRatio> poolSizeRatios
    ) const {
        std::vector<VkDescriptorPoolSize> poolSizes{};
        poolSizes.reserve(poolSizeRatios.size());
        for (const auto &[type, ratio]: poolSizeRatios)
            poolSizes.emplace_back(VkDescriptorPoolSize{
                .type = type,
                .descriptorCount = static_cast<uint32_t>(ratio * static_cast<float>(setCount))
            });

        spdlog::trace(
            "Expanding descriptor pool with {} sizes",
            poolSizes.size()
        );

        return std::make_shared<VulkanDescriptorPoolFixed>(
            device,
            poolSizes,
            setCount
        );
    }

    std::shared_ptr<VulkanDescriptorPoolFixed> VulkanDescriptorPoolExpanding::getPool() {
        std::shared_ptr<VulkanDescriptorPoolFixed> pool;

        if (!readyPools.empty()) {
            spdlog::trace("Popping pool from ready descriptor pools");
            pool = readyPools.back();
            readyPools.pop_back();
        } else {
            spdlog::trace("Exhausted ready descriptor pools, creating new pool");
            pool = createPool(setsPerPool, ratios);

            setsPerPool = static_cast<uint32_t>(setsPerPool * 1.5);
            // TODO: This needs to respect the physical device limits, instead of this magic number
            if (setsPerPool > 4092)
                setsPerPool = 4092;
        }

        return pool;
    }

    VulkanDescriptorPoolExpanding::VulkanDescriptorPoolExpanding(
        const std::shared_ptr<VulkanDevice> &device,
        const uint32_t maxSets,
        const std::span<PoolSizeRatio> poolSizeRatios
    ) : device(device),
        setsPerPool(static_cast<uint32_t>(static_cast<float>(maxSets) * scaler)) {
        for (const auto &poolSizeRatio: poolSizeRatios)
            ratios.push_back(poolSizeRatio);

        readyPools.emplace_back(createPool(maxSets, poolSizeRatios));
    }

    VulkanDescriptorPoolExpanding::VulkanDescriptorPoolExpanding(VulkanDescriptorPoolExpanding &&fp) noexcept
        : device(std::exchange(fp.device, nullptr)),
          setsPerPool(fp.setsPerPool) {}

    VulkanDescriptorPoolExpanding &VulkanDescriptorPoolExpanding::operator
    =(VulkanDescriptorPoolExpanding &&fp) noexcept {
        std::swap(device, fp.device);
        std::swap(setsPerPool, fp.setsPerPool);

        return *this;
    }

    void VulkanDescriptorPoolExpanding::reset() {
        spdlog::trace("Resetting all descriptor pools");
        for (const auto &pool: readyPools)
            pool->reset();

        for (const auto &pool: fullPools) {
            pool->reset();
            readyPools.push_back(pool);
        }
        fullPools.clear();
    }

    std::shared_ptr<VulkanDescriptorSet> VulkanDescriptorPoolExpanding::allocate(
        const VulkanDescriptorSetLayout &layout) {
        auto pool = getPool();
        std::shared_ptr<VulkanDescriptorSet> descriptorSet;

        try {
            descriptorSet = std::make_shared<VulkanDescriptorSet>(pool->allocate(layout));
        } catch (const OutOfPoolMemoryException &) {
            fullPools.push_back(pool);
            pool = getPool();
            descriptorSet = std::make_shared<VulkanDescriptorSet>(pool->allocate(layout));
        }

        readyPools.push_back(pool);

        spdlog::trace("Allocated new descriptor set from pool");

        return descriptorSet;
    }
}
