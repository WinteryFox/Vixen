#pragma once

#include "command/CommandBuffer.h"
#include "command/CommandPool.h"
#include "command/Fence.h"
#include "command/Semaphore.h"

namespace Vixen {
    struct Frame {
        CommandPool* commandPool;
        CommandBuffer* commandBuffer;
        Semaphore* semaphore;
        Fence* fence;
        bool fenceSignaled;
    };
}
