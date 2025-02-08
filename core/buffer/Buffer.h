#pragma once

#include <cstdint>

#include "BufferUsage.h"

namespace Vixen {
    class Buffer {
        BufferUsage usage;

        uint32_t count;

        uint32_t stride;

    public:
        Buffer(BufferUsage usage, uint32_t count, uint32_t stride);

        virtual ~Buffer() = default;

        [[nodiscard]] BufferUsage getUsage() const;

        [[nodiscard]] uint32_t getCount() const;

        [[nodiscard]] uint32_t getStride() const;

        [[nodiscard]] uint64_t getSize() const;
    };
}
