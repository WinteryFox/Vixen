#pragma once

#include <memory>
#include "VkCommandPool.h"

namespace Vixen::Vk {
    class VkCommandBuffer {
    public:
        enum class Level {
            PRIMARY = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            SECONDARY = VK_COMMAND_BUFFER_LEVEL_SECONDARY
        };

    private:
        std::shared_ptr<VkCommandPool> commandPool;

        ::VkCommandBuffer commandBuffer;

    public:
        VkCommandBuffer(const std::shared_ptr<VkCommandPool> &commandPool, ::VkCommandBuffer commandBuffer);

        VkCommandBuffer(const std::shared_ptr<VkCommandPool> &commandPool, Level level);

        VkCommandBuffer(VkCommandBuffer &&o) noexcept;

        ~VkCommandBuffer();

        static std::vector<VkCommandBuffer>
        allocateCommandBuffers(const std::shared_ptr<VkCommandPool> &commandPool, Level level, uint32_t count);

    private:
        static std::vector<::VkCommandBuffer>
        allocateCommandBuffers(const std::shared_ptr<VkCommandPool> &commandPool, VkCommandBufferLevel level, uint32_t count);
    };
}
