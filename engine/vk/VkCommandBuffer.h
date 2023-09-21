#pragma once

#include <memory>
#include "VkCommandPool.h"

namespace Vixen::Vk {
    class VkCommandBuffer {
        std::shared_ptr<VkCommandPool> commandPool;

        ::VkCommandBuffer commandBuffer;

    public:
        VkCommandBuffer(const std::shared_ptr<VkCommandPool> &commandPool, ::VkCommandBuffer commandBuffer);

        VkCommandBuffer(const std::shared_ptr<VkCommandPool> &commandPool, VkCommandBufferLevel level);

        VkCommandBuffer(const VkCommandBuffer &) = delete;

        VkCommandBuffer &operator=(const VkCommandBuffer &) = delete;

        ~VkCommandBuffer();
    };
}
