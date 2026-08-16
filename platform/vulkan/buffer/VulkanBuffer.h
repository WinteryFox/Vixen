#pragma once

#include <cstdint>
#include <volk.h>

#include "buffer/BufferUsage.h"
#include "core/buffer/Buffer.h"

typedef VmaAllocation_T *VmaAllocation;

namespace Vixen {
    struct VulkanBuffer final : Buffer {
        VulkanBuffer(
            const BufferUsageFlags usage,
            const uint32_t count,
            const uint32_t stride,
            VkBuffer buffer,
            VmaAllocation allocation
        ) : Buffer(usage, count, stride),
            buffer(buffer),
            allocation(allocation) {
        }

        VkBuffer buffer;

        VmaAllocation allocation;
    };
}
