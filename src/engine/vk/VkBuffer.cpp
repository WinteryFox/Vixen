#include "VkBuffer.h"

#include "Device.h"

namespace Vixen::Vk {
    VkBuffer::VkBuffer(const std::shared_ptr<Device>& device, const Usage bufferUsage, const size_t& size)
        : Buffer(bufferUsage, size),
          device(device),
          allocation(VK_NULL_HANDLE),
          buffer(VK_NULL_HANDLE),
          data(nullptr) {
        VmaAllocationCreateFlags allocationFlags = 0;
        VkBufferUsageFlags bufferUsageFlags = 0;
        VkMemoryPropertyFlags requiredFlags = 0;

        if (bufferUsage & Usage::VERTEX)
            bufferUsageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        if (bufferUsage & Usage::INDEX)
            bufferUsageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        if (bufferUsage & Usage::TRANSFER_DST) {
            bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        if (bufferUsage & Usage::TRANSFER_SRC) {
            allocationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }

        if (bufferUsage & Usage::UNIFORM) {
            allocationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            bufferUsageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }

        const VkBufferCreateInfo bufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = size,
            .usage = bufferUsageFlags,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr
        };

        const VmaAllocationCreateInfo allocationCreateInfo = {
            // TODO: Only add this flag if necessary (e.g. staging buffer)
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
                nullptr
            ),
            "Failed to create Vk buffer"
        );

        if (allocationFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
            data = map();
    }

    VkBuffer::VkBuffer(VkBuffer&& o) noexcept
        : Buffer(o.bufferUsage, o.size),
          allocation(std::exchange(o.allocation, nullptr)),
          buffer(std::exchange(o.buffer, nullptr)),
          data(std::exchange(o.data, nullptr)) {}

    VkBuffer::~VkBuffer() {
        unmap();
        vmaDestroyBuffer(device->getAllocator(), buffer, allocation);
    }

    void VkBuffer::write(const std::byte* data, const size_t dataSize, const size_t offset) {
        if (!this->data)
            throw std::runtime_error("This buffer is not mapped and thus not writable");
        if (offset + dataSize > size)
            throw std::runtime_error("Buffer overflow");

        memcpy(this->data + offset, data, dataSize);
    }

    ::VkBuffer VkBuffer::getBuffer() const {
        return buffer;
    }

    std::byte* VkBuffer::map() {
        void* data;
        checkVulkanResult(
            vmaMapMemory(device->getAllocator(), allocation, &data),
            "Failed to map buffer"
        );

        return static_cast<std::byte*>(data);
    }

    void VkBuffer::unmap() {
        if (data)
            vmaUnmapMemory(device->getAllocator(), allocation);
    }
}
