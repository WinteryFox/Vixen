#include "VulkanDescriptorSet.h"

#include <core/exception/OutOfPoolMemoryException.h>

#include "VulkanDescriptorPoolFixed.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanDevice.h"
#include "buffer/VulkanBuffer.h"
#include "image/VulkanImageView.h"

namespace Vixen {
    VulkanDescriptorSet::VulkanDescriptorSet(
        const std::shared_ptr<VulkanDevice>& device,
        const std::shared_ptr<const VulkanDescriptorPoolFixed>& pool,
        const VulkanDescriptorSetLayout& layout
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

    VulkanDescriptorSet::VulkanDescriptorSet(VulkanDescriptorSet&& other) noexcept
        : set(std::exchange(other.set, nullptr)),
          device(std::move(other.device)),
          pool(std::move(other.pool)) {}

    VulkanDescriptorSet& VulkanDescriptorSet::operator=(VulkanDescriptorSet&& other) noexcept {
        std::swap(other.set, set);
        std::swap(other.device, device);
        std::swap(other.pool, pool);

        return *this;
    }

    VulkanDescriptorSet::~VulkanDescriptorSet() {
        if (set != VK_NULL_HANDLE)
            checkVulkanResult(
                vkFreeDescriptorSets(device->getDevice(), pool->getPool(), 1, &set),
                "Failed to free descriptor set"
            );
    }

    void VulkanDescriptorSet::writeUniformBuffer(
        const uint32_t binding,
        const VulkanBuffer& buffer,
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

    void VulkanDescriptorSet::writeCombinedImageSampler(
        const uint32_t binding,
        const VulkanImageView& view
    ) const {
        const VkDescriptorImageInfo imageInfo{
            .sampler = view.getSampler(),
            .imageView = view.getImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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

    ::VkDescriptorSet VulkanDescriptorSet::getSet() const {
        return set;
    }
}
