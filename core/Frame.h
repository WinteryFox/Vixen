#pragma once

#include "Swapchain.h"
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
        std::vector<Semaphore*> waitSemaphores;
        std::vector<Swapchain*> swapchainsToPresent;
        std::vector<Semaphore*> transferSemaphores;
    };
}
