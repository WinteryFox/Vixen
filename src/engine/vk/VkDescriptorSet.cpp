#include "VkDescriptorSet.h"

#include "VkDescriptorPoolFixed.h"
#include "VkDescriptorSetLayout.h"
#include "exception/OutOfPoolMemoryException.h"

namespace Vixen::Vk {
    VkDescriptorSet::VkDescriptorSet(
        const std::shared_ptr<Device>& device,
        const std::shared_ptr<const VkDescriptorPoolFixed>& pool,
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

        switch (const auto& result = vkAllocateDescriptorSets(device->getDevice(), &info, &set)) {
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            throw OutOfPoolMemoryException("Descriptor pool is out of memory");
        case VK_ERROR_FRAGMENTED_POOL:
            // TODO: We should probably attempt to defragment the pool here instead of just erroring
            throw OutOfPoolMemoryException("Descriptor pool is fragmented");
        default:
            checkVulkanResult(
                result,
                "Failed to allocate descriptor set"
            );
            break;
        }
    }

    VkDescriptorSet::VkDescriptorSet(VkDescriptorSet&& other) noexcept
        : set(std::exchange(other.set, nullptr)),
          device(std::move(other.device)),
          pool(std::move(other.pool)) {}

    VkDescriptorSet& VkDescriptorSet::operator=(VkDescriptorSet&& other) noexcept {
        std::swap(other.set, set);
        std::swap(other.device, device);
        std::swap(other.pool, pool);

        return *this;
    }

    VkDescriptorSet::~VkDescriptorSet() {
        if (set != VK_NULL_HANDLE)
            checkVulkanResult(
                vkFreeDescriptorSets(device->getDevice(), pool->getPool(), 1, &set),
                "Failed to free descriptor set"
            );
    }

    void VkDescriptorSet::updateUniformBuffer(
        const uint32_t binding,
        const VkBuffer& buffer,
        const uint32_t offset,
        const uint32_t size
    ) const {
        if (offset + size > buffer.getSize())
            throw std::runtime_error("Offset plus size is greater than buffer size");

        const VkDescriptorBufferInfo bufferInfo{
            .buffer = buffer.getBuffer(),
            .offset = offset,
            .range = size
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

    void VkDescriptorSet::updateCombinedImageSampler(
        const uint32_t binding,
        const VkSampler& sampler,
        const VkImageView& view
    ) const {
        const VkDescriptorImageInfo imageInfo{
            .sampler = sampler.getSampler(),
            .imageView = view.getImageView(),
            .imageLayout = view.getImage()->getLayout()
        };

        const VkWriteDescriptorSet write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = set,
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        };

        vkUpdateDescriptorSets(device->getDevice(), 1, &write, 0, nullptr);
    }

    ::VkDescriptorSet VkDescriptorSet::getSet() const {
        return set;
    }
}
