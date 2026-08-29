#pragma once

#include <cstdint>

#include "BufferUsage.h"

namespace Vixen {
    class Buffer {
        BufferUsageFlags usage;

        uint64_t size;

    public:
        Buffer(BufferUsageFlags usage, uint64_t size);

        virtual ~Buffer() = default;

        [[nodiscard]] BufferUsageFlags getUsage() const;

        [[nodiscard]] uint64_t getSize() const;
    };
}
