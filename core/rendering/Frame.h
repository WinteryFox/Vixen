#pragma once

#include <vector>

#include "core/memory/DeferredRelease.h"
#include "Swapchain.h"
#include "core/command/CommandBuffer.h"
#include "core/command/CommandPool.h"
#include "core/command/Fence.h"
#include "core/command/Semaphore.h"

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
