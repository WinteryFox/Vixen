#pragma once

#include <cstdio>
#include <cstdint>
#include <cstring>

namespace Vixen::Vk {
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

    /**
     * A persistently mapped, coherent, synchronized, host or server data buffer.
     */
    class Buffer {
    public:
        Buffer &operator=(const Buffer &) = delete;

        virtual void write(const void *data, size_t dataSize, size_t offset) = 0;

        [[nodiscard]] size_t getSize() const;

        [[nodiscard]] BufferUsage getBufferUsage() const;

    protected:
        const std::size_t size;

        const BufferUsage bufferUsage;

        /**
         * Create a new buffer. Where the allocation is made is determined by the allocation usage.
         * @param size The size of this buffer measured in bytes.
         * @param bufferUsage Specifies how the buffer will be used and what data it will hold.
         * @param allocationUsage Specifies how this buffer's allocated memory will be used.
         */
        Buffer(const std::size_t &size, BufferUsage bufferUsage);

        virtual void *map() = 0;

        virtual void unmap() = 0;
    };
}
