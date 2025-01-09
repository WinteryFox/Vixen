#pragma once

#include "core/RenderingDevice.h"

namespace Vixen {
    class OpenGLRenderingDevice final : public RenderingDevice {
    public:
        OpenGLRenderingDevice();

        ~OpenGLRenderingDevice() override;

        Swapchain *createSwapchain(Surface *surface) override;

        void resizeSwapchain(CommandQueue *commandQueue, Swapchain *swapchain, uint32_t imageCount) override;

        void destroySwapchain(Swapchain *swapchain) override;

        Fence *createFence() override;

        void waitOnFence(const Fence *fence) override;

        void destroyFence(Fence *fence) override;

        Semaphore *createSemaphore() override;

        void destroySemaphore(Semaphore *semaphore) override;

        CommandPool *createCommandPool(uint32_t queueFamily, CommandBufferType type) override;

        void resetCommandPool(CommandPool *pool) override;

        void destroyCommandPool(CommandPool *pool) override;

        CommandBuffer *createCommandBuffer(CommandPool *pool) override;

        void beginCommandBuffer(CommandBuffer *commandBuffer) override;

        void endCommandBuffer(CommandBuffer *commandBuffer) override;

        Buffer *createBuffer(BufferUsage usage, uint32_t count, uint32_t stride) override;

        void destroyBuffer(Buffer *buffer) override;

        uint32_t getQueueFamily(QueueFamilyFlags queueFamilyFlags, Surface *surface) override;

        CommandQueue *createCommandQueue() override;

        void executeCommandQueueAndPresent(CommandQueue *commandQueue, const std::vector<Semaphore *> &waitSemaphores,
                                           const std::vector<CommandBuffer *> &commandBuffers,
                                           const std::vector<Semaphore *> &semaphores,
                                           Fence *fence, const std::vector<Swapchain *> &swapchains) override;

        void destroyCommandQueue(CommandQueue *commandQueue) override;

        Image *createImage(const ImageFormat &format, const ImageView &view) override;

        std::byte *mapImage(Image *image) override;

        void unmapImage(Image *image) override;

        void destroyImage(Image *image) override;

        Sampler *createSampler(SamplerState state) override;

        void destroySampler(Sampler *sampler) override;

        Shader *createShaderFromSpirv(const std::string &name, const std::vector<ShaderStageData> &stages) override;

        void destroyShaderModules(Shader *shader) override;

        void destroyShader(Shader *shader) override;
    };
}
