#include "VkDescriptorPool.h"

namespace Vixen::Vk {
    VkDescriptorPool::VkDescriptorPool(
        const std::shared_ptr<Device>& device,
        const std::vector<VkDescriptorPoolSize>& sizes,
        const uint32_t maxSets
    ) : device(device),
        pool(VK_NULL_HANDLE) {
        const VkDescriptorPoolCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .maxSets = maxSets,
            .poolSizeCount = static_cast<uint32_t>(sizes.size()),
            .pPoolSizes = sizes.data()
        };

        checkVulkanResult(
            vkCreateDescriptorPool(device->getDevice(), &info, nullptr, &pool),
            "Failed to create descriptor pool"
        );
    }

    VkDescriptorPool::VkDescriptorPool(VkDescriptorPool&& fp) noexcept
        : pool(std::exchange(fp.pool, nullptr)),
          device(std::move(fp.device)) {}

    VkDescriptorPool const& VkDescriptorPool::operator=(VkDescriptorPool&& fp) noexcept {
        std::swap(fp.pool, pool);
        std::swap(fp.device, device);

        return *this;
    }

    VkDescriptorPool::~VkDescriptorPool() {
        vkDestroyDescriptorPool(device->getDevice(), pool, nullptr);
    }

    VkDescriptorSet VkDescriptorPool::allocate(const VkDescriptorSetLayout &layout) const {
        const auto &l = layout.getLayout();
        const VkDescriptorSetAllocateInfo info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &l
        };

        VkDescriptorSet set;
        checkVulkanResult(
            vkAllocateDescriptorSets(device->getDevice(), &info, &set),
            "Failed to allocate descriptor set"
        );

        return set;
    }

    std::shared_ptr<Device> VkDescriptorPool::getDevice() const {
        return device;
    }

    ::VkDescriptorPool VkDescriptorPool::getPool() const {
        return pool;
    }
}
