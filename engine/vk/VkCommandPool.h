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

        VkCommandPool(const VkCommandPool &) = delete;

        VkCommandPool &operator=(const VkCommandPool &) = delete;

        ~VkCommandPool();

        std::vector<VkCommandBuffer> allocateCommandBuffers(VkCommandBuffer::Level level, uint32_t count);

        VkCommandBuffer allocateCommandBuffer(VkCommandBuffer::Level level);

        [[nodiscard]] ::VkCommandPool getCommandPool() const;
    };
}
