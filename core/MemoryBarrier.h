#pragma once

#include "BarrierAccessFlags.h"

namespace Vixen {
    struct MemoryBarrier {
        BarrierAccessFlags sourceAccess;
        BarrierAccessFlags targetAccess;
    };
}
