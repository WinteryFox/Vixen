#pragma once

#include "core/buffer/Buffer.h"

typedef struct VmaAllocation_T *VmaAllocation;
class VulkanRenderingDevice;
enum class BufferUsage : uint32_t;

namespace Vixen {
    struct VulkanBuffer final : Buffer {
        VulkanBuffer(
            const BufferUsage usage,
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
