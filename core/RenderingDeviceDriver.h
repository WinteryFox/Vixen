#pragma once

#include <expected>
#include <string>

#include "BufferBarrier.h"
#include "ClearValue.h"
#include "Framebuffer.h"
#include "ImageBarrier.h"
#include "IndexFormat.h"
#include "MemoryBarrier.h"
#include "PipelineStageFlags.h"
#include "QueueFamilyFlags.h"
#include "RenderPass.h"
#include "Surface.h"
#include "Swapchain.h"
#include "command/CommandPool.h"
#include "command/CommandBuffer.h"
#include "buffer/Buffer.h"
#include "buffer/BufferCopyRegion.h"
#include "buffer/BufferImageCopyRegion.h"
#include "buffer/BufferUsage.h"
#include "command/CommandQueue.h"
#include "command/Fence.h"
#include "command/Semaphore.h"
#include "error/Error.h"
#include "error/SwapchainError.h"
#include "image/Image.h"
#include "image/ImageCopyRegion.h"
#include "image/ImageFormat.h"
#include "image/ImageLayout.h"
#include "image/ImageSubresourceRange.h"
#include "image/ImageView.h"
#include "image/Sampler.h"
#include "image/SamplerState.h"
#include "shader/Shader.h"
#include "shader/ShaderLanguage.h"
#include "shader/ShaderStageData.h"

namespace Vixen {
    class RenderingDeviceDriver {
    protected:
        static bool reflectShader(const std::vector<ShaderStageData> &stages, Shader *shader);

    public:
        virtual ~RenderingDeviceDriver() = default;

        virtual auto createSwapchain(Surface *surface) -> std::expected<Swapchain *, Error> = 0;

        virtual void resizeSwapchain(CommandQueue *commandQueue, Swapchain *swapchain, uint32_t imageCount) = 0;

        virtual auto acquireSwapchainFramebuffer(CommandQueue *commandQueue, Swapchain *swapchain)
            -> std::expected<Framebuffer *, SwapchainError> = 0;

        virtual void destroySwapchain(Swapchain *swapchain) = 0;

        virtual Fence *createFence() = 0;

        virtual auto waitOnFence(Fence *fence) -> std::expected<void, Error> = 0;

        virtual void destroyFence(Fence *fence) = 0;

        virtual Semaphore *createSemaphore() = 0;

        virtual void destroySemaphore(Semaphore *semaphore) = 0;

        virtual CommandPool *createCommandPool(uint32_t queueFamily, CommandBufferType type) = 0;

        virtual void resetCommandPool(CommandPool *pool) = 0;

        virtual void destroyCommandPool(CommandPool *pool) = 0;

        virtual CommandBuffer *createCommandBuffer(CommandPool *pool) = 0;

        virtual void beginCommandBuffer(CommandBuffer *commandBuffer) = 0;

        virtual void endCommandBuffer(CommandBuffer *commandBuffer) = 0;

        virtual Buffer *createBuffer(BufferUsage usage, uint32_t count, uint32_t stride) = 0;

        virtual void destroyBuffer(Buffer *buffer) = 0;

        virtual auto getQueueFamily(QueueFamilyFlags queueFamilyFlags,
                                    Surface *surface) -> std::expected<uint32_t, Error> = 0;

        virtual auto createCommandQueue(uint32_t queueFamilyIndex) -> std::expected<CommandQueue *, Error> = 0;

        virtual void executeCommandQueueAndPresent(CommandQueue *commandQueue,
                                                   const std::vector<Semaphore *> &waitSemaphores,
                                                   const std::vector<CommandBuffer *> &commandBuffers,
                                                   const std::vector<Semaphore *> &semaphores,
                                                   Fence *fence, const std::vector<Swapchain *> &swapchains) = 0;

        virtual void destroyCommandQueue(CommandQueue *commandQueue) = 0;

        virtual Image *createImage(const ImageFormat &format, const ImageView &view) = 0;

        virtual std::byte *mapImage(Image *image) = 0;

        virtual void unmapImage(Image *image) = 0;

        virtual void destroyImage(Image *image) = 0;

        virtual Sampler *createSampler(SamplerState state) = 0;

        virtual void destroySampler(Sampler *sampler) = 0;

        virtual std::vector<std::byte> compileSpirvFromSource(ShaderStage stage, const std::string &source,
                                                              ShaderLanguage language);

        virtual Shader *createShaderFromSpirv(const std::string &name,
                                              const std::vector<ShaderStageData> &stages) = 0;

        virtual void destroyShaderModules(Shader *shader) = 0;

        virtual void destroyShader(Shader *shader) = 0;

        virtual void commandBeginRenderPass(CommandBuffer *commandBuffer,
                                            RenderPass *renderPass,
                                            Framebuffer *framebuffer,
                                            CommandBufferType commandBufferType,
                                            const glm::uvec2 &rectangle,
                                            const std::vector<ClearValue> &clearValues) = 0;

        virtual void commandEndRenderPass(CommandBuffer *commandBuffer) = 0;

        virtual void commandSetViewport(CommandBuffer *commandBuffer, const std::vector<glm::uvec2> &viewports) = 0;

        virtual void commandSetScissor(CommandBuffer *commandBuffer, const std::vector<glm::uvec2> &scissors) = 0;

        virtual void commandBindVertexBuffers(CommandBuffer *commandBuffer,
                                              uint32_t count,
                                              const std::vector<Buffer *> &buffers,
                                              const std::vector<uint64_t> &offsets) = 0;

        virtual void commandBindIndexBuffers(CommandBuffer *commandBuffer,
                                             Buffer *buffer,
                                             IndexFormat format,
                                             uint64_t offset) = 0;

        virtual void commandPipelineBarrier(CommandBuffer *commandBuffer,
                                            PipelineStageFlags sourceStages,
                                            PipelineStageFlags destinationStages,
                                            const std::vector<MemoryBarrier> &memoryBarriers,
                                            const std::vector<BufferBarrier> &bufferBarriers,
                                            const std::vector<ImageBarrier> &imageBarriers) = 0;

        virtual void commandClearBuffer(CommandBuffer *commandBuffer, Buffer *buffer, uint64_t offset,
                                        uint64_t size) = 0;

        virtual void commandCopyBuffer(CommandBuffer *commandBuffer, Buffer *source, Buffer *destination,
                                       const std::vector<BufferCopyRegion> &regions) = 0;

        virtual void commandCopyImage(CommandBuffer *commandBuffer, Image *source, ImageLayout sourceLayout,
                                      Image *destination, ImageLayout destinationLayout,
                                      const std::vector<ImageCopyRegion> &regions) = 0;

        virtual void commandResolveImage(CommandBuffer *commandBuffer, Image *source, ImageLayout sourceLayout,
                                         uint32_t sourceLayer, uint32_t sourceMipmap, Image *destination,
                                         ImageLayout destinationLayout, uint32_t destinationLayer,
                                         uint32_t destinationMipmap) = 0;

        virtual void commandClearColorImage(CommandBuffer *commandBuffer, Image *image, ImageLayout imageLayout,
                                            const glm::vec4 &color, const ImageSubresourceRange &subresource) = 0;

        virtual void commandCopyBufferToImage(CommandBuffer *commandBuffer, Buffer *buffer, Image *image,
                                              ImageLayout layout,
                                              const std::vector<BufferImageCopyRegion> &regions) = 0;

        virtual void commandCopyImageToBuffer(CommandBuffer *commandBuffer, Image *image, ImageLayout layout,
                                              Buffer *buffer, const std::vector<BufferImageCopyRegion> &regions) = 0;

        virtual void commandBeginLabel(CommandBuffer *commandBuffer, const std::string &label,
                                       const glm::vec3 &color) = 0;

        virtual void commandEndLabel(CommandBuffer *commandBuffer) = 0;
    };
}
