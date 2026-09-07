#pragma once

#include <volk.h>
#include <vector>
#include "core/command/CommandBuffer.h"

#include "core/command/Fence.h"

namespace Vixen {
    struct VulkanCommandQueue;

    struct VulkanFence final : Fence {
        VkFence fence;
        VulkanCommandQueue* queueSignaledFrom = nullptr;
        bool submitted = false;
        std::vector<std::shared_ptr<std::atomic<CommandBuffer::State>>> commandSubmissions;
    };
}
