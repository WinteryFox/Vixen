#pragma once

#include "VkCommandBuffer.h"

namespace Vixen::Vk {
    class VkCommandPool {
    public:
        enum class Usage {
            GRAPHICS,
            TRANSIENT
        };

    private:
        ::VkDevice device;

        ::VkCommandPool commandPool;

        static std::vector<::VkCommandBuffer> allocateCommandBuffers(
            ::VkDevice device,
            ::VkCommandPool commandPool,
            VkCommandBufferLevel level,
            uint32_t count
        );

    public:
        VkCommandPool(::VkDevice device, uint32_t queueFamilyIndex, Usage usage, bool createReset);

        VkCommandPool(const VkCommandPool&) = delete;

        VkCommandPool& operator=(const VkCommandPool&) = delete;

        VkCommandPool(VkCommandPool&& other) noexcept;

        VkCommandPool const& operator=(VkCommandPool&& other) noexcept;

        ~VkCommandPool();

        std::vector<VkCommandBuffer> allocate(VkCommandBuffer::Level level, uint32_t count);

        VkCommandBuffer allocate(VkCommandBuffer::Level level);

        void reset() const;
    };
}
