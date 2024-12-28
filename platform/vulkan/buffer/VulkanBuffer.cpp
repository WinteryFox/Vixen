#include "VulkanBuffer.h"

#include <utility>

namespace Vixen {
    VulkanBuffer::VulkanBuffer(
        const std::shared_ptr<VulkanRenderingDevice> &device,
        const BufferUsage usage,
        const uint32_t count,
        const uint32_t stride
    ) : device(device),
        count(count),
        stride(stride),
        allocation(nullptr),
        buffer(VK_NULL_HANDLE),
        allocationInfo(),
        usage(usage) {
        if (count <= 0)
            throw std::runtime_error("Count cannot be equal to or less than 0");
        if (stride <= 0)
            throw std::runtime_error("Stride cannot be equal to or less than 0");

        VmaAllocationCreateFlags allocationFlags = 0;
        VkBufferUsageFlags bufferUsageFlags = 0;
        VkMemoryPropertyFlags requiredFlags = 0;

        if (usage & Vertex)
            bufferUsageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        if (usage & Index)
            bufferUsageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        if (usage & BufferUsage::CopySource) {
            allocationFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT |
                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }

        if (usage & BufferUsage::CopyDestination) {
            bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        if (usage & Uniform) {
            allocationFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT |
                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            bufferUsageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }

        const VkBufferCreateInfo bufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = count * stride,
            .usage = bufferUsageFlags,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr
        };

        const VmaAllocationCreateInfo allocationCreateInfo = {
            .flags = allocationFlags,
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = requiredFlags,
            .preferredFlags = 0,
            .memoryTypeBits = 0,
            .pool = nullptr,
            .pUserData = nullptr,
            .priority = 0.0f
        };

        ASSERT_THROW(
            vmaCreateBuffer(
                device->getAllocator(),
                &bufferCreateInfo,
                &allocationCreateInfo,
                &buffer,
                &allocation,
                &allocationInfo
            ),
            CantCreateError,
            "Failed to create buffer"
        );
    }

    VulkanBuffer::VulkanBuffer(VulkanBuffer &&other) noexcept
        : device(std::exchange(other.device, nullptr)),
          count(other.count),
          stride(other.stride),
          allocation(std::exchange(other.allocation, nullptr)),
          buffer(std::exchange(other.buffer, nullptr)),
          allocationInfo(other.allocationInfo),
          usage(other.usage) {}

    VulkanBuffer &VulkanBuffer::operator=(VulkanBuffer &&other) noexcept {
        std::swap(device, other.device);
        std::swap(count, other.count);
        std::swap(stride, other.stride);
        std::swap(allocation, other.allocation);
        std::swap(buffer, other.buffer);
        std::swap(allocationInfo, other.allocationInfo);
        std::swap(usage, other.usage);

        return *this;
    }

    VulkanBuffer::~VulkanBuffer() {
        if (device != nullptr)
            vmaDestroyBuffer(device->getAllocator(), buffer, allocation);
    }

    void VulkanBuffer::setData(const std::byte *data) const {
        if (!data)
            throw std::runtime_error("Data is nullptr");

        memcpy(allocationInfo.pMappedData, data, getSize());
    }

    ::VkBuffer VulkanBuffer::getBuffer() const { return buffer; }

    std::size_t VulkanBuffer::getSize() const { return count * stride; }

    uint32_t VulkanBuffer::getCount() const { return count; }

    uint32_t VulkanBuffer::getStride() const { return stride; }
}
