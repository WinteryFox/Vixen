#pragma once

#include <volk.h>

#include "core/command/CommandBuffer.h"

namespace Vixen {
    struct VulkanCommandBuffer final : CommandBuffer {
        VkCommandBuffer commandBuffer;
    };
}
