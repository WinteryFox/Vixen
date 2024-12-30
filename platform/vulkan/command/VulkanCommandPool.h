#pragma once

#include <volk.h>

#include "core/command/CommandPool.h"

namespace Vixen {
    struct VulkanCommandPool final : CommandPool {
        VkCommandPool pool;
    };
}
