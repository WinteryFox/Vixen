#pragma once

#include <vector>

#include "DeferredRelease.h"
#include "Swapchain.h"
#include "command/CommandBuffer.h"
#include "command/CommandPool.h"
#include "command/Fence.h"
#include "command/Semaphore.h"

namespace Vixen {
    struct Frame {
        CommandPool* commandPool;
        CommandBuffer* commandBuffer;
        Fence* fence;
        bool fenceSignaled;
        std::vector<DeferredRelease> deferredReleases;
        std::vector<Semaphore*> waitSemaphores;
        std::vector<Swapchain*> swapchainsToPresent;
    };
}
