#pragma once

#include <memory>
#include <vma/vk_mem_alloc.h>

#include "../Vulkan.h"

namespace Vixen {
    enum class BufferUsage : std::uint32_t;
    class VulkanDevice;

    class VulkanBuffer {
        std::shared_ptr<VulkanDevice> device;

        uint32_t count;

        uint32_t stride;

        VmaAllocation allocation;

        VkBuffer buffer;

        VmaAllocationInfo allocationInfo;

        BufferUsage usage;

    public:
        VulkanBuffer(const std::shared_ptr<VulkanDevice>& device, BufferUsage usage, uint32_t count, uint32_t stride);

        VulkanBuffer(const VulkanBuffer&) = delete;

        VulkanBuffer& operator=(const VulkanBuffer&) = delete;

        VulkanBuffer(VulkanBuffer&& other) noexcept;

        VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

        ~VulkanBuffer();

        void setData(const std::byte* data) const;

        [[nodiscard]] ::VkBuffer getBuffer() const;

        [[nodiscard]] std::size_t getSize() const;

        [[nodiscard]] uint32_t getCount() const;

        [[nodiscard]] uint32_t getStride() const;
    };
}
