#include "VkBuffer.h"

namespace Vixen::Vk {
    VkBuffer::VkBuffer(const std::shared_ptr<Device> &device, Usage bufferUsage, const size_t &size)
            : Buffer(bufferUsage, size),
              device(device),
              allocation(VK_NULL_HANDLE),
              buffer(VK_NULL_HANDLE) {
        VkBufferUsageFlags bufferUsageFlags;
        if (bufferUsage & Usage::VERTEX)
            bufferUsageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if (bufferUsage & Usage::INDEX)
            bufferUsageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        if (bufferUsage & Usage::TRANSFER_DST)
            bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        if (bufferUsage & Usage::TRANSFER_SRC)
            bufferUsageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        if (bufferUsage & Usage::UNIFORM)
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
            : Buffer(o.bufferUsage, o.size),
              allocation(std::exchange(o.allocation, nullptr)),
              buffer(std::exchange(o.buffer, nullptr)) {}

    VkBuffer::~VkBuffer() {
        vmaDestroyBuffer(device->getAllocator(), buffer, allocation);
    }

    void VkBuffer::write(const char *data, size_t dataSize, size_t offset) {
        if (offset + dataSize > size)
            throw std::runtime_error("Buffer overflow");

        void *d = map();
        memcpy(static_cast<char *>(d) + offset, data, dataSize);
        unmap();
    }

    char *VkBuffer::map() {
        void *data;
        checkVulkanResult(
                vmaMapMemory(device->getAllocator(), allocation, &data),
                "Failed to map buffer"
        );

        return static_cast<char *>(data);
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
        device->getTransferCommandPool()
                ->allocateCommandBuffer(VkCommandBuffer::Level::PRIMARY)
                .record(
                        VkCommandBuffer::Usage::SINGLE,
                        [this, &destination, &destinationOffset](auto commandBuffer) {
                            VkBufferCopy region{
                                    .srcOffset = 0,
                                    .dstOffset = destinationOffset,
                                    .size = size,
                            };

                            vkCmdCopyBuffer(commandBuffer, buffer, destination.getBuffer(), 1, &region);
                        }
                )
                .submit(device->getTransferQueue(), {}, {}, {});
    }

    VkBuffer VkBuffer::stage(
            const std::shared_ptr<Device> &device,
            Usage usage,
            size_t size,
            const char *data
    ) {
        auto source = VkBuffer(device, usage | Usage::TRANSFER_SRC, size);
        auto destination = VkBuffer(device, usage | Usage::TRANSFER_DST, size);

        source.write(data, size, 0);
        source.transfer(destination, 0);

        return destination;
    }
}
