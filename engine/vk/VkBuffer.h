#include <memory>
#include "../Buffer.h"
#include "Vulkan.h"
#include "Device.h"

namespace Vixen::Vk {
    class VkBuffer : public Buffer {
    protected:
        std::shared_ptr<Device> device;

        VmaAllocation allocation;

        ::VkBuffer buffer;

    public:
        VkBuffer(const std::shared_ptr<Device> &device, const size_t &size, BufferUsage bufferUsage);

        VkBuffer(const VkBuffer &) = delete;

        VkBuffer &operator=(const VkBuffer &) = delete;

        VkBuffer(VkBuffer &&o) noexcept;

        ~VkBuffer();

        void write(const void *data, size_t dataSize, size_t offset) override;

        [[nodiscard]] ::VkBuffer getBuffer() const;

    protected:
        void *map() override;

        void unmap() override;
    };
}
