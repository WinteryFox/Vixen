#include "VkBuffer.h"

#include "Device.h"

namespace Vixen::Vk {
    VkBuffer::VkBuffer(): count(0), stride(0), allocation(nullptr), buffer(nullptr), allocationInfo(), usage() {}

    VkBuffer::VkBuffer(
        const std::shared_ptr<Device> &device,
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
            error("Count cannot be equal to or less than 0");
        if (stride <= 0)
            error("Stride cannot be equal to or less than 0");

        VmaAllocationCreateFlags allocationFlags = 0;
        VkBufferUsageFlags bufferUsageFlags = 0;
        VkMemoryPropertyFlags requiredFlags = 0;

        if (usage & BufferUsage::Vertex)
            bufferUsageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        if (usage & BufferUsage::Index)
            bufferUsageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        if (usage & BufferUsage::CopySource) {
            allocationFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT |
                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }

        if (usage & BufferUsage::CopyDestination) {
            bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        if (usage & BufferUsage::Uniform) {
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

        checkVulkanResult(
            vmaCreateBuffer(
                device->getAllocator(),
                &bufferCreateInfo,
                &allocationCreateInfo,
                &buffer,
                &allocation,
                &allocationInfo
            ),
            "Failed to create buffer"
        );
    }

    VkBuffer::VkBuffer(VkBuffer &&other) noexcept
        : device(std::exchange(other.device, nullptr)),
          count(other.count),
          stride(other.stride),
          allocation(std::exchange(other.allocation, nullptr)),
          buffer(std::exchange(other.buffer, nullptr)),
          allocationInfo(other.allocationInfo),
          usage(other.usage) {}

    VkBuffer &VkBuffer::operator=(VkBuffer &&other) noexcept {
        std::swap(device, other.device);
        std::swap(count, other.count);
        std::swap(stride, other.stride);
        std::swap(allocation, other.allocation);
        std::swap(buffer, other.buffer);
        std::swap(allocationInfo, other.allocationInfo);
        std::swap(usage, other.usage);

        return *this;
    }

    VkBuffer::~VkBuffer() {
        if (device != nullptr)
            vmaDestroyBuffer(device->getAllocator(), buffer, allocation);
    }

    void VkBuffer::setData(const std::byte *data) const {
        if (!data)
            throw std::runtime_error("Data is nullptr");

        memcpy(allocationInfo.pMappedData, data, getSize());
    }

    ::VkBuffer VkBuffer::getBuffer() const { return buffer; }

    std::size_t VkBuffer::getSize() const { return count * stride; }

    uint32_t VkBuffer::getCount() const { return count; }

    uint32_t VkBuffer::getStride() const { return stride; }
}
