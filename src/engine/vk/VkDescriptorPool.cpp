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

    VkDescriptorPool::~VkDescriptorPool() {
        vkDestroyDescriptorPool(device->getDevice(), pool, nullptr);
    }
}
