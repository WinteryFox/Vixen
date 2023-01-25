#include "../Buffer.h"

namespace Vixen::Engine {
    class VkBuffer : virtual Buffer {
    public:
        VkBuffer(const size_t &size, BufferUsage bufferUsage, AllocationUsage allocationUsage);
    };
}
