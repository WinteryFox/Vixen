#include <vma/vk_mem_alloc.h>
#include "../Buffer.h"
#include "Macro.h"

namespace Vixen::Engine {
    class VkBuffer : virtual Buffer {
        VmaAllocation allocation;

        ::VkBuffer buffer;

    public:
        VkBuffer(const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage);

        ~VkBuffer();
    };
}
