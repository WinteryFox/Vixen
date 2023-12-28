#pragma once

#include "VkBuffer.h"
#include "VkFence.h"
#include "VkImage.h"
#include "../CommandBuffer.h"

namespace Vixen::Vk {
    class VkCommandPool;

    class VkCommandBuffer {
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
        VkCommandBuffer& record(CommandBufferUsage usage, F commands) {
            reset();

            begin(usage);

            commands(commandBuffer);

            end();

            return *this;
        }

        void wait() const;

        void reset() const;

        void begin(CommandBufferUsage usage) const;

        void end() const;

        void submit(
            ::VkQueue queue,
            const std::vector<::VkSemaphore>& waitSemaphores,
            const std::vector<::VkPipelineStageFlags>& waitMasks,
            const std::vector<::VkSemaphore>& signalSemaphores
        ) const;

        void copyBuffer(const VkBuffer& source, const VkBuffer& destination) const;

        void copyImage(const VkImage& source, const VkImage& destination) const;

        void copyBufferToImage(const VkBuffer& source, const VkImage& destination) const;

        void transitionImage(const VkImage& image, VkImageLayout layout) const;
    };
}
