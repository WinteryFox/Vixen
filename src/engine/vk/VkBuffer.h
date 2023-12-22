#pragma once

#include <memory>
#include <vk_mem_alloc.h>

#include "Vulkan.h"
#include "../Buffer.h"

namespace Vixen::Vk {
    class Device;

    class VkBuffer final : public Buffer {
        std::shared_ptr<Device> device;

        VmaAllocation allocation;

        ::VkBuffer buffer;

        std::byte* data;

    public:
        VkBuffer(const std::shared_ptr<Device>& device, Usage bufferUsage, const size_t& size);

        VkBuffer(const VkBuffer&) = delete;

        VkBuffer& operator=(const VkBuffer&) = delete;

        VkBuffer(VkBuffer&& o) noexcept;

        ~VkBuffer() override;

        void write(const std::byte* data, size_t dataSize, size_t offset) override;

        [[nodiscard]] ::VkBuffer getBuffer() const;

    private:
        std::byte* map() override;

        void unmap() override;
    };
}
