#pragma once

#include <volk.h>

#include "core/command/Fence.h"

namespace Vixen {
    struct VulkanCommandQueue;

    struct VulkanFence final : Fence {
        VkFence fence;
        VulkanCommandQueue* queueSignaledFrom;
    };
}
