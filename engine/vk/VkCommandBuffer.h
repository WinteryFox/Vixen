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

        enum class Usage {
            SINGLE = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            SIMULTANEOUS = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
            RENDER_PASS_CONTINUE = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT
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

        template<typename F>
        VkCommandBuffer &record(F commands, Usage usage) {
            VkCommandBufferBeginInfo info{
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                    .flags = static_cast<VkCommandBufferUsageFlags>(usage)
            };

            checkVulkanResult(
                    vkBeginCommandBuffer(commandBuffer, &info),
                    "Failed to begin command buffer"
            );

            commands(commandBuffer);

            checkVulkanResult(
                    vkEndCommandBuffer(commandBuffer),
                    "Failed to end command buffer"
            );

            return *this;
        }

        [[nodiscard]] ::VkCommandBuffer getCommandBuffer() const;

    private:
        static std::vector<::VkCommandBuffer>
        allocateCommandBuffers(const std::shared_ptr<VkCommandPool> &commandPool, VkCommandBufferLevel level, uint32_t count);
    };
}
