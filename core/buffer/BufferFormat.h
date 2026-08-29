#pragma once

#include <cstdint>

#include "BufferUsage.h"

namespace Vixen {
    struct BufferFormat {
        uint64_t size;

        BufferUsageFlags usage;
    };
}
