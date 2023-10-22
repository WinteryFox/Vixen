#include "VkBuffer.h"

namespace Vixen::Vk {
    VkBuffer::VkBuffer(const std::shared_ptr<Device> &device, const size_t &size, BufferUsage bufferUsage)
            : Buffer(size, bufferUsage),
              device(device),
              allocation(VK_NULL_HANDLE),
              buffer(VK_NULL_HANDLE) {
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

        VmaAllocationCreateInfo allocationCreateInfo = {
                // TODO: Only add this flag if necessary (e.g. staging buffer)
                .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO
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
    }

    VkBuffer::VkBuffer(VkBuffer &&o) noexcept
            : Buffer(o.size, o.bufferUsage),
              allocation(std::exchange(o.allocation, nullptr)),
              buffer(std::exchange(o.buffer, nullptr)) {}

    VkBuffer::~VkBuffer() {
        vmaDestroyBuffer(device->getAllocator(), buffer, allocation);
    }

    void VkBuffer::write(const void *data, size_t dataSize, size_t offset) {
        if (offset + dataSize > size)
            throw std::runtime_error("Buffer overflow");

        void *d = map();
        memcpy(static_cast<char *>(d) + offset, data, dataSize);
        unmap();
    }

    void *VkBuffer::map() {
        void *data = nullptr;
        checkVulkanResult(
                vmaMapMemory(device->getAllocator(), allocation, &data),
                "Failed to map buffer"
        );

        return data;
    }

    void VkBuffer::unmap() {
        vmaUnmapMemory(device->getAllocator(), allocation);
    }

    ::VkBuffer VkBuffer::getBuffer() const {
        return buffer;
    }

    void VkBuffer::transfer(
            VkBuffer &destination,
            size_t destinationOffset
    ) {
        device->getTransferCommandPool()->allocateCommandBuffer(VkCommandBuffer::Level::PRIMARY).record(
                [this, &destination, &destinationOffset](auto commandBuffer) {
                    VkBufferCopy region{
                            .srcOffset = 0,
                            .dstOffset = destinationOffset,
                            .size = size,
                    };

                    vkCmdCopyBuffer(commandBuffer, buffer, destination.getBuffer(), 1, &region);
                },
                VkCommandBuffer::Usage::SINGLE
        ).submit(device->getTransferQueue());
    }

    VkBuffer VkBuffer::stage(
            const std::shared_ptr<Device> &device,
            const void *data,
            size_t size,
            BufferUsage usage
    ) {
        auto source = VkBuffer(device, size, usage | BufferUsage::TRANSFER_SRC);
        auto destination = VkBuffer(device, size, usage | BufferUsage::TRANSFER_DST);

        source.write(data, size, 0);
        source.transfer(destination, 0);

        return destination;
    }
}
