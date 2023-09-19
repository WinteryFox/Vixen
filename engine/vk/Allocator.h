#include <volk.h>
#include <vk_mem_alloc.h>

namespace Vixen::Vk {
    class Allocator {
        friend class VkBuffer;

        VmaAllocator allocator;

    public:
        Allocator(VkPhysicalDevice gpu, VkDevice device, VkInstance instance);

        ~Allocator();
    };
}
