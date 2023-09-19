#pragma once

#include "Buffer.h"

namespace Vixen::Vk {
    class WritableBuffer : public Buffer {
    public:
        WritableBuffer(const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage);

        /**
         * Writes data to this buffer with a specified size and offset.
         * @param data Pointer to the data to write to this buffer.
         * @param dataSize The size of the data measured in bytes.
         * @param offset The offset from the buffer to write to measured in bytes.
         * @return Returns this object.
         */
        virtual WritableBuffer &write(const void *data, std::size_t dataSize, std::size_t offset) = 0;

        /**
         * Writes a vector of data to this buffer with a specified offset.
         * @param data The vector of data to write to this buffer.
         * @param offset The offset from the buffer to write to measured in bytes.
         * @return Returns this object.
         */
        template<typename T>
        WritableBuffer &write(const std::vector<T> &data, std::size_t offset) {
            return write(data.data(), data.size() * sizeof(T), offset);
        }
    };
}
