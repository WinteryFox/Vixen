#pragma once

#include <cstdio>
#include <cstdint>
#include <cstring>

namespace Vixen::Vk {
    /**
     * A persistently mapped, coherent, synchronized, host or server data buffer.
     */
    class Buffer {
    public:
        enum class Usage : std::uint32_t {
            VERTEX = 1 << 0,
            INDEX = 1 << 1,
            UNIFORM = 1 << 2,
            TRANSFER_DST = 1 << 3,
            TRANSFER_SRC = 1 << 4,
        };

        Buffer &operator=(const Buffer &) = delete;

        /**
         * Map the buffer, write to it and unmap the buffer from host memory.
         * @param data A pointer pointing to the start of the data.
         * @param dataSize The size of the data.
         * @param offset The offset within this buffer to start writing from.
         */
        virtual void write(const char *data, size_t dataSize, size_t offset) = 0;

        [[nodiscard]] size_t getSize() const;

        [[nodiscard]] Usage getBufferUsage() const;

    protected:
        const Usage bufferUsage;

        const std::size_t size;

        /**
         * Create a new buffer. Where the allocation is made is determined by the allocation usage.
         * @param bufferUsage Specifies how the buffer will be used and what data it will hold.
         * @param size The size of this buffer measured in bytes.
         * @param allocationUsage Specifies how this buffer's allocated memory will be used.
         */
        Buffer(Usage bufferUsage, const std::size_t &size);

        virtual char *map() = 0;

        virtual void unmap() = 0;
    };

    inline Buffer::Usage operator|(Buffer::Usage a, Buffer::Usage b) {
        return static_cast<Buffer::Usage>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
    }

    inline bool operator&(Buffer::Usage a, Buffer::Usage b) {
        return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b);
    }
}
