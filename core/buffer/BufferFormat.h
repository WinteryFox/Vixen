#pragma once
#include "BufferUsage.h"

namespace Vixen {
    struct BufferFormat {
        uint32_t size;

        BufferUsageFlags usage;
    };
}
