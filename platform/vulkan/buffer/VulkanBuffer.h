#pragma once

#include <cstdint>
#include <volk.h>

#include "buffer/BufferUsage.h"
#include "core/buffer/Buffer.h"

struct VmaAllocation_T;
typedef VmaAllocation_T *VmaAllocation;

namespace Vixen {
    struct VulkanBuffer final : Buffer {
        VulkanBuffer(
            const BufferUsageFlags usage,
            const uint64_t size,
            const VkBuffer buffer,
            const VmaAllocation allocation
        ) : Buffer(usage, size),
            buffer(buffer),
            allocation(allocation) {
        }

        VkBuffer buffer;

        VmaAllocation allocation;
    };
}
