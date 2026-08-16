#pragma once

#include <cstdint>

#include "BufferUsage.h"

namespace Vixen {
    struct BufferFormat {
        uint32_t count;

        uint32_t stride;

        BufferUsageFlags usage;

        [[nodiscard]] uint64_t getSize() const noexcept {
            return static_cast<uint64_t>(count) * stride;
        }
    };
}
