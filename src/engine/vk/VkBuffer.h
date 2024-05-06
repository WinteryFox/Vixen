#pragma once

#include <memory>
#include <vma/vk_mem_alloc.h>

#include "Vulkan.h"
#include "../Buffer.h"

namespace Vixen::Vk {
    class Device;

    class VkBuffer {
        std::shared_ptr<Device> device;

        uint32_t count;

        uint32_t stride;

        VmaAllocation allocation;

        ::VkBuffer buffer;

        VmaAllocationInfo allocationInfo;

        BufferUsage usage;

    public:
        VkBuffer();

        VkBuffer(const std::shared_ptr<Device>& device, BufferUsage usage, uint32_t count, uint32_t stride);

        VkBuffer(const VkBuffer&) = delete;

        VkBuffer& operator=(const VkBuffer&) = delete;

        VkBuffer(VkBuffer&& other) noexcept;

        VkBuffer& operator=(VkBuffer&& other) noexcept;

        ~VkBuffer();

        void setData(const std::byte* data) const;

        [[nodiscard]] ::VkBuffer getBuffer() const;

        [[nodiscard]] std::size_t getSize() const;

        [[nodiscard]] uint32_t getCount() const;

        [[nodiscard]] uint32_t getStride() const;
    };
}
