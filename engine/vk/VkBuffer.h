#pragma once

#include <memory>
#include "../Buffer.h"
#include "Vulkan.h"
#include "Device.h"

namespace Vixen::Vk {
    class VkBuffer : public Buffer {
    protected:
        std::shared_ptr<Device> device;

        VmaAllocation allocation;

        ::VkBuffer buffer;

    public:
        VkBuffer(const std::shared_ptr<Device> &device, BufferUsage bufferUsage, const size_t &size);

        VkBuffer(const VkBuffer &) = delete;

        VkBuffer &operator=(const VkBuffer &) = delete;

        VkBuffer(VkBuffer &&o) noexcept;

        ~VkBuffer();

        void *map() override;

        void unmap() override;

        void write(const void *data, size_t dataSize, size_t offset) override;

        /**
         * Copies data from one buffer to another.
         * @param destination The destination buffer to copy the data to.
         * @param destinationOffset The offset from the destination buffer to copy into.
         */
        void transfer(VkBuffer &destination, size_t destinationOffset);

        [[nodiscard]] ::VkBuffer getBuffer() const;

        /**
         * Transfers data from host memory to a host local buffer and uploads that to a device local buffer.
         * @param device The device to create the buffers on.
         * @param usage The usage flags for the resulting buffer. Do not include any TRANSFER usage flags.
         * @param size The size of the data.
         * @param data Pointer to the start of the data.
         * @return Returns the resulting device local buffer.
         */
        static VkBuffer stage(const std::shared_ptr<Device> &device, BufferUsage usage, size_t size, const void *data);

        template<typename F>
        static VkBuffer stage(
                const std::shared_ptr<Device> &device,
                BufferUsage usage,
                size_t size,
                F lambda
        ) {
            auto source = VkBuffer(device, usage | BufferUsage::TRANSFER_SRC, size);
            auto destination = VkBuffer(device, usage | BufferUsage::TRANSFER_DST, size);

            void *data = source.map();
            lambda(data);
            source.unmap();
            source.transfer(destination, 0);

            return destination;
        }
    };
}
