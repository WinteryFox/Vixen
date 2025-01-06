#pragma once

#include "core/command/Fence.h"

namespace Vixen {
    struct VulkanFence : Fence {
        VkFence fence;
    };
}
