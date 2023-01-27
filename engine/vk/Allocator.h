#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

namespace Vixen::Engine {
    class Allocator {
        friend class VkBuffer;

        VmaAllocator allocator;

    public:
        Allocator(VkPhysicalDevice gpu, VkDevice device, VkInstance instance);

        ~Allocator();
    };
}
