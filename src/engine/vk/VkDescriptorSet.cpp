#include "VkDescriptorSet.h"

namespace Vixen::Vk {
    VkDescriptorSet::VkDescriptorSet(
        const std::shared_ptr<Device>& device,
        const std::shared_ptr<VkDescriptorPool>& pool,
        const VkDescriptorSetLayout& layout
    ) : set(VK_NULL_HANDLE),
        device(device),
        pool(pool) {
        const auto& l = layout.getLayout();
        const VkDescriptorSetAllocateInfo info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = pool->getPool(),
            .descriptorSetCount = 1,
            .pSetLayouts = &l
        };

        checkVulkanResult(
            vkAllocateDescriptorSets(device->getDevice(), &info, &set),
            "Failed to allocate descriptor set"
        );
    }

    VkDescriptorSet::VkDescriptorSet(VkDescriptorSet&& fp) noexcept
        : set(std::exchange(fp.set, nullptr)),
          device(std::move(device)),
          pool(std::move(pool)) {}

    VkDescriptorSet const& VkDescriptorSet::operator=(VkDescriptorSet&& fp) noexcept {
        std::swap(fp.set, set);
        std::swap(fp.device, device);
        std::swap(fp.pool, pool);

        return *this;
    }

    VkDescriptorSet::~VkDescriptorSet() {
        checkVulkanResult(
            vkFreeDescriptorSets(device->getDevice(), pool->getPool(), 1, &set),
            "Failed to free descriptor set"
        );
    }

    void VkDescriptorSet::updateUniformBuffer(const uint32_t binding, const VkBuffer& buffer) const {
        const VkDescriptorBufferInfo bufferInfo{
            .buffer = buffer.getBuffer(),
            // TODO: This can't be hardcoded in the future to deal with buffers storing
            // multiple descriptors (or other things), will need a better solution
            .offset = 0,
            .range = buffer.getSize()
        };

        const VkWriteDescriptorSet write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = set,
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &bufferInfo,
            .pTexelBufferView = nullptr
        };

        vkUpdateDescriptorSets(device->getDevice(), 1, &write, 0, nullptr);
    }

    ::VkDescriptorSet VkDescriptorSet::getSet() const {
        return set;
    }
}
