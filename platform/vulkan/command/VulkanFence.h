#pragma once

#include "core/command/Fence.h"

namespace Vixen {
    struct VulkanFence final : Fence {
        VkFence fence;
    };
}
