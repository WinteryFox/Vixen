#include <memory>
#include "Allocator.h"
#include "../Buffer.h"
#include "Vulkan.h"

namespace Vixen::Vk {
    class VkBuffer : virtual Buffer {
    protected:
        std::shared_ptr<Allocator> allocator;

        VmaAllocation allocation;

        ::VkBuffer buffer;

    public:
        VkBuffer(const std::shared_ptr<Allocator> &allocator, const size_t &size, BufferUsage bufferUsage,
                 AllocationUsage allocationUsage);

        ~VkBuffer();
    };
}
