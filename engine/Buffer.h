#pragma once

#include <cstdio>
#include <vector>
#include <cstdint>
#include <cstring>

namespace Vixen::Engine {
    enum class AllocationUsage {
        GPU_ONLY,
        CPU_ONLY,
        CPU_TO_GPU,
        GPU_TO_CPU,
        /*CPU_COPY,
        GPU_LAZY_ALLOCATED,*/
    };

    enum class BufferUsage : std::uint32_t {
        VERTEX = 1 << 0,
        INDEX = 1 << 1,
        UNIFORM = 1 << 2,
        TRANSFER_DST = 1 << 3,
        TRANSFER_SRC = 1 << 4,
    };

    inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
        return static_cast<BufferUsage>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
    }

    inline bool operator&(BufferUsage a, BufferUsage b) {
        return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b);
    }

    template<typename T>
    class Buffer {
    public:
        const std::size_t size;

        const BufferUsage bufferUsage;

        const AllocationUsage allocationUsage;

    protected:
        /**
         * Create a new buffer. Where the allocation is made is determined by the allocation usage.
         * @param size The size of this buffer measured in T.
         * @param bufferUsage Specifies how the buffer will be used and what data it will hold.
         * @param allocationUsage Specifies how this buffer's allocated memory will be used.
         */
        Buffer(const std::size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage)
                : size(size), bufferUsage(bufferUsage), allocationUsage(allocationUsage) {}

    public:
        Buffer(const Buffer &) = delete;

        Buffer &operator=(const Buffer &) = delete;

        /**
         * Map this buffer's data into CPU memory in read/write mode.
         * @return Returns a pointer to the mapped data.
         */
        virtual T *map() = 0;

        /**
         * Unmap this buffer's data from CPU memory and write it back to the buffer.
         */
        virtual void unmap() = 0;

        /**
         * Writes data to this buffer with a specified size and offset.
         * @param data Pointer to the data to write to this buffer.
         * @param dataSize The size of the data measured in <T>.
         * @param offset The offset from the buffer to write to measured in <T>.
         * @return Returns this object.
         */
        virtual Buffer &write(const T *data, std::size_t dataSize, std::size_t offset) {
            T *mapped = map();
            std::memcpy(mapped + offset, data, dataSize * sizeof(T));
            unmap();
            return *this;
        }

        /**
         * Writes a vector of data to this buffer with a specified offset.
         * @param data The vector of data to write to this buffer.
         * @param offset The offset from the buffer to write to measured in <T>.
         * @return Returns this object.
         */
        virtual Buffer &write(const std::vector<T> &data, std::size_t offset) {
            return write(data.data(), data.size(), offset);
        }
    };
}
