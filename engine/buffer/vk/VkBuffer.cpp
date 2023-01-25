#include "VkBuffer.h"

namespace Vixen::Engine {
    VkBuffer::VkBuffer(const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage)
            : Buffer(size, bufferUsage, allocationUsage) {
        //checkVulkanResult(vmaCreateBuffer(), "Failed to create Vk buffer");
    }
}
