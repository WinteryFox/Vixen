#pragma once

#include "VkFence.h"
#include "VkImage.h"
#include "VkImageView.h"
#include "VkMesh.h"
#include "../AttachmentInfo.h"
#include "../CommandBuffer.h"
#include "../Rectangle.h"

namespace Vixen::Vk {
    class VkCommandPool;

    class VkCommandBuffer {
        std::shared_ptr<VkCommandPool> commandPool;

        ::VkCommandBuffer commandBuffer;

        VkFence fence;

    public:
        VkCommandBuffer(const std::shared_ptr<VkCommandPool> &commandPool, ::VkCommandBuffer commandBuffer);

        VkCommandBuffer(const VkCommandBuffer &) = delete;

        VkCommandBuffer &operator=(const VkCommandBuffer &) = delete;

        VkCommandBuffer(VkCommandBuffer &&other) noexcept;

        VkCommandBuffer &operator=(VkCommandBuffer &&other) noexcept;

        ~VkCommandBuffer();

        template<typename F>
        void record(F commands) const {
            commands(commandBuffer);
        }

        void wait() const;

        void reset() const;

        void begin(CommandBufferUsage usage) const;

        void end() const;

        void submit(
            ::VkQueue queue,
            const std::vector<::VkSemaphore> &waitSemaphores,
            const std::vector<::VkPipelineStageFlags> &waitMasks,
            const std::vector<::VkSemaphore> &signalSemaphores
        ) const;

        void beginRenderPass(
            uint32_t width,
            uint32_t height,
            uint8_t samples,
            const std::vector<AttachmentInfo> &attachments,
            const VkImageView &depthAttachment
        ) const;

        void endRenderPass() const;

        void setViewport(Rectangle rectangle) const;

        void setScissor(Rectangle rectangle) const;

        void drawMesh(const glm::mat4 &modelMatrix, const VkMesh &mesh) const;

        void copyBuffer(const VkBuffer &source, const VkBuffer &destination) const;

        void copyBufferToImage(const VkBuffer &source, const VkImage &destination) const;

        void copyImage(const VkImage &source, const VkImage &destination) const;

        void transitionImage(VkImage &image, VkImageLayout layout) const;

        void blitImage(VkImage &source, const VkImage &destination) const;
    };
}
