#pragma once

#include "Buffer.h"
#include "WritableBuffer.h"

namespace Vixen::Engine {
    class ReadableBuffer : public virtual Buffer {
    public:
        ReadableBuffer(const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage);

        /**
         * Reads from this buffer
         * @param buffer Where to write the read data to, must be at least {@code size} big.
         * @param size The size in bytes to read from this buffer.
         * @param offset The offset in bytes to add to the start of the buffer for the read.
         */
        virtual void* read(void* destination, std::size_t size, std::size_t offset) = 0;

        /**
         * Transfers (copies) the data from this buffer into another destination buffer.
         * @param destination The destination buffer to transfer the data into.
         */
        virtual void transfer(WritableBuffer &destination) = 0;
    };
}
