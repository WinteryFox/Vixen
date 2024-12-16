#pragma once

#include <memory>

#include "synchronization/VulkanFence.h"

namespace Vixen {
    struct AttachmentInfo;
    struct Rectangle;
    class VulkanCommandPool;
    class VulkanBuffer;
    class VulkanImage;
    class VulkanImageView;
    class VulkanMesh;
    enum class CommandBufferUsage;

    class VulkanCommandBuffer {
        std::shared_ptr<VulkanCommandPool> commandPool;

        VkCommandBuffer commandBuffer;

        VulkanFence fence;

    public:
        VulkanCommandBuffer(const std::shared_ptr<VulkanCommandPool> &commandPool, ::VkCommandBuffer commandBuffer);

        VulkanCommandBuffer(const VulkanCommandBuffer &) = delete;

        VulkanCommandBuffer &operator=(const VulkanCommandBuffer &) = delete;

        VulkanCommandBuffer(VulkanCommandBuffer &&other) noexcept;

        VulkanCommandBuffer &operator=(VulkanCommandBuffer &&other) noexcept;

        ~VulkanCommandBuffer();

        VkCommandBuffer getCommandBuffer() const;

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
            const VulkanImageView &depthAttachment
        ) const;

        void endRenderPass() const;

        void setViewport(const Rectangle &rectangle) const;

        void setScissor(const Rectangle &rectangle) const;

        void drawMesh(const glm::mat4 &transform, const VulkanMesh &mesh) const;

        void copyBuffer(const VulkanBuffer &source, const VulkanBuffer &destination) const;

        void copyBufferToImage(const VulkanBuffer &source, const VulkanImage &destination) const;

        void copyImage(const VulkanImage &source, const VulkanImage &destination) const;

        void transitionImage(VulkanImage &image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel,
                             uint32_t mipLevels) const;

        void blitImage(const VulkanImage &source, VulkanImage &destination) const;
    };
}
