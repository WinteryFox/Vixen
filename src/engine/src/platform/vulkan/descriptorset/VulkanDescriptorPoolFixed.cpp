#include "VulkanDescriptorPoolFixed.h"

#include "Vulkan.h"
#include "VulkanDevice.h"

namespace Vixen {
    VulkanDescriptorPoolFixed::VulkanDescriptorPoolFixed(
        const std::shared_ptr<VulkanDevice>& device,
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

    VulkanDescriptorPoolFixed::VulkanDescriptorPoolFixed(VulkanDescriptorPoolFixed&& other) noexcept
        : device(std::move(other.device)),
          pool(other.pool) {}

    VulkanDescriptorPoolFixed& VulkanDescriptorPoolFixed::operator=(VulkanDescriptorPoolFixed&& other) noexcept {
        std::swap(other.pool, pool);
        std::swap(other.device, device);

        return *this;
    }

    VulkanDescriptorPoolFixed::~VulkanDescriptorPoolFixed() {
        vkDestroyDescriptorPool(device->getDevice(), pool, nullptr);
    }

    VulkanDescriptorSet VulkanDescriptorPoolFixed::allocate(const VulkanDescriptorSetLayout& layout) const {
        return {device, shared_from_this(), layout};
    }

    void VulkanDescriptorPoolFixed::reset() const {
        vkResetDescriptorPool(device->getDevice(), pool, 0);
    }

    std::shared_ptr<VulkanDevice> VulkanDescriptorPoolFixed::getDevice() const {
        return device;
    }

    ::VkDescriptorPool VulkanDescriptorPoolFixed::getPool() const {
        return pool;
    }
}
