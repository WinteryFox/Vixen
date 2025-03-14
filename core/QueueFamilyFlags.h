#pragma once

#include "Bitmask.h"

namespace Vixen {
    enum class QueueFamilyFlags : uint32_t {
        Graphics = 1 << 0,
        Transfer = 1 << 1,
        Compute = 1 << 2
    };

    DECLARE_BITMASK(QueueFamilyFlags);
}
