#pragma once

#include <cstdio>
#include <vector>

namespace Vixen::Engine {
    enum class BufferUsage {
        GPU_ONLY,
        CPU_ONLY,
        CPU_TO_GPU,
        GPU_TO_CPU,
        /*CPU_COPY,
        GPU_LAZY_ALLOCATED,*/
    };

    enum class BufferType {
        TRANSFER_DST,
        TRANSFER_SRC,
        ARRAY,
        //TEXTURE,
        UNIFORM,
    };

    template<typename T>
    class Buffer {
    public:
        std::size_t size{};

        BufferUsage usage;

        BufferType type;

    protected:
        explicit Buffer(const std::size_t size, BufferUsage usage, BufferType type) : size(size), usage(usage),
                                                                                      type(type) {}

    public:
        Buffer(const Buffer &) = delete;

        Buffer &operator=(const Buffer &) = delete;

        virtual void write(const T *data, std::size_t size, std::size_t offset) = 0;

        void write(const std::vector<T> &data, std::size_t offset) {
            write(data.data(), data.size(), offset);
        }

        virtual T *map() = 0;

        virtual T *unmap() = 0;
    };
}
