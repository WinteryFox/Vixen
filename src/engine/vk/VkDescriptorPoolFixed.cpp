#include "VkDescriptorPoolFixed.h"

namespace Vixen::Vk {
    VkDescriptorPoolFixed::VkDescriptorPoolFixed(
        const std::shared_ptr<Device>& device,
        const std::vector<VkDescriptorPoolSize>& sizes,
        const uint32_t maxSets
    ) : device(device),
        pool(VK_NULL_HANDLE) {
        const VkDescriptorPoolCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = maxSets,
            .poolSizeCount = static_cast<uint32_t>(sizes.size()),
            .pPoolSizes = sizes.data()
        };

        checkVulkanResult(
            vkCreateDescriptorPool(device->getDevice(), &info, nullptr, &pool),
            "Failed to create descriptor pool"
        );
    }

    VkDescriptorPoolFixed::VkDescriptorPoolFixed(VkDescriptorPoolFixed&& other) noexcept
        : device(std::move(other.device)),
          pool(other.pool) {}

    VkDescriptorPoolFixed& VkDescriptorPoolFixed::operator=(VkDescriptorPoolFixed&& other) noexcept {
        std::swap(other.pool, pool);
        std::swap(other.device, device);

        return *this;
    }

    VkDescriptorPoolFixed::~VkDescriptorPoolFixed() {
        vkDestroyDescriptorPool(device->getDevice(), pool, nullptr);
    }

    VkDescriptorSet VkDescriptorPoolFixed::allocate(const VkDescriptorSetLayout& layout) const {
        return {device, shared_from_this(), layout};
    }

    void VkDescriptorPoolFixed::reset() const {
        vkResetDescriptorPool(device->getDevice(), pool, 0);
    }

    std::shared_ptr<Device> VkDescriptorPoolFixed::getDevice() const {
        return device;
    }

    ::VkDescriptorPool VkDescriptorPoolFixed::getPool() const {
        return pool;
    }
}
