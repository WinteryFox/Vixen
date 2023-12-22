#pragma once

#include "VkBuffer.h"
#include "VkFence.h"
#include "VkImage.h"

namespace Vixen::Vk {
    class VkCommandPool;

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

        VkFence fence;

    public:
        VkCommandBuffer(const std::shared_ptr<VkCommandPool>& commandPool, ::VkCommandBuffer commandBuffer);

        VkCommandBuffer(const VkCommandBuffer&) = delete;

        VkCommandBuffer& operator=(const VkCommandBuffer&) = delete;

        VkCommandBuffer(VkCommandBuffer&& other) noexcept;

        VkCommandBuffer& operator=(VkCommandBuffer&& other) noexcept;

        ~VkCommandBuffer();

        template <typename F>
        VkCommandBuffer& record(Usage usage, F commands) {
            reset();

            begin(usage);

            commands(commandBuffer);

            end();

            return *this;
        }

        VkCommandBuffer& wait();

        VkCommandBuffer& reset();

        VkCommandBuffer& begin(Usage usage);

        VkCommandBuffer& end();

        VkCommandBuffer& copyBuffer(const VkBuffer& source, const VkBuffer& destination);

        VkCommandBuffer& copyImage(const VkImage& source, const VkImage& destination);

        VkCommandBuffer& copyBufferToImage(const VkBuffer& source, const VkImage& destination);

        void submit(
            ::VkQueue queue,
            const std::vector<::VkSemaphore>& waitSemaphores,
            const std::vector<::VkPipelineStageFlags>& waitMasks,
            const std::vector<::VkSemaphore>& signalSemaphores
        );
    };
}
