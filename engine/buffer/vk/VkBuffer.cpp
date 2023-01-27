#include "VkBuffer.h"

namespace Vixen::Engine {
    VkBuffer::VkBuffer(const std::shared_ptr<Allocator>& allocator, const size_t &size, BufferUsage bufferUsage,
                       AllocationUsage allocationUsage)
            : Buffer(size, bufferUsage, allocationUsage), allocation(VK_NULL_HANDLE), buffer(VK_NULL_HANDLE),
              allocator(allocator) {
        VkBufferUsageFlags bufferUsageFlags;
        if (bufferUsage & BufferUsage::VERTEX)
            bufferUsageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if (bufferUsage & BufferUsage::INDEX)
            bufferUsageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        if (bufferUsage & BufferUsage::TRANSFER_DST)
            bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        if (bufferUsage & BufferUsage::TRANSFER_SRC)
            bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        if (bufferUsage & BufferUsage::UNIFORM)
            bufferUsageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        VkBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = size;
        bufferCreateInfo.usage = bufferUsageFlags;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaMemoryUsage memoryUsage;
        switch (allocationUsage) {
            case AllocationUsage::GPU_ONLY:
                memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
                break;
            case AllocationUsage::CPU_ONLY:
                memoryUsage = VMA_MEMORY_USAGE_CPU_COPY;
                break;
            case AllocationUsage::CPU_TO_GPU:
                memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
                break;
            case AllocationUsage::GPU_TO_CPU:
                memoryUsage = VMA_MEMORY_USAGE_GPU_TO_CPU;
                break;
        }

        VmaAllocationCreateInfo allocationCreateInfo = {};
        allocationCreateInfo.usage = memoryUsage;

        checkVulkanResult(
                vmaCreateBuffer(allocator->allocator, &bufferCreateInfo, &allocationCreateInfo, &buffer, &allocation,
                                nullptr),
                "Failed to create Vk buffer"
        );
    }

    VkBuffer::~VkBuffer() {
        vmaDestroyBuffer(allocator->allocator, buffer, allocation);
    }
}
