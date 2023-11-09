#pragma once

#include "VkFence.h"

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
        ::VkDevice device;

        ::VkCommandPool commandPool;

        ::VkCommandBuffer commandBuffer;

        VkFence fence;

    public:
        VkCommandBuffer(::VkDevice device, ::VkCommandPool commandPool, ::VkCommandBuffer commandBuffer);

        VkCommandBuffer(VkCommandBuffer &) = delete;

        VkCommandBuffer &operator=(VkCommandBuffer &) = delete;

        VkCommandBuffer(VkCommandBuffer &&o) noexcept = default;

        ~VkCommandBuffer();

        template<typename F>
        VkCommandBuffer &record(Usage usage, F commands) {
            VkCommandBufferBeginInfo info{
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                    .flags = static_cast<VkCommandBufferUsageFlags>(usage)
            };

            reset();

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

        VkCommandBuffer &wait();

        VkCommandBuffer &reset();

        VkCommandBuffer &submit(
                ::VkQueue queue,
                const std::vector<::VkSemaphore> &waitSemaphores,
                const std::vector<::VkPipelineStageFlags> &waitMasks,
                const std::vector<::VkSemaphore> &signalSemaphores
        );
    };
}
