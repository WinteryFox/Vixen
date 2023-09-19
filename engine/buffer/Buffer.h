#pragma once

#include <cstdio>
#include <vector>
#include <cstdint>
#include <cstring>

namespace Vixen::Vk {
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

    /**
     * A persistently mapped, coherent, synchronized, host or server data buffer.
     */
    class Buffer {
    public:
        const std::size_t size;

        const BufferUsage bufferUsage;

        const AllocationUsage allocationUsage;

        Buffer(const Buffer &) = delete;

        Buffer &operator=(const Buffer &) = delete;

    protected:
        /**
         * Create a new buffer. Where the allocation is made is determined by the allocation usage.
         * @param size The size of this buffer measured in bytes.
         * @param bufferUsage Specifies how the buffer will be used and what data it will hold.
         * @param allocationUsage Specifies how this buffer's allocated memory will be used.
         */
        Buffer(const std::size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage);
    };
}
