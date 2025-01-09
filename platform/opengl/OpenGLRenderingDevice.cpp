#include "OpenGLRenderingDevice.h"

namespace Vixen {
    OpenGLRenderingDevice::OpenGLRenderingDevice() = default;

    OpenGLRenderingDevice::~OpenGLRenderingDevice() = default;

    Swapchain *OpenGLRenderingDevice::createSwapchain(Surface *surface) {
    }

    void OpenGLRenderingDevice::resizeSwapchain(CommandQueue *commandQueue, Swapchain *swapchain, uint32_t imageCount) {
    }

    void OpenGLRenderingDevice::destroySwapchain(Swapchain *swapchain) {
    }

    Fence *OpenGLRenderingDevice::createFence() {
    }

    void OpenGLRenderingDevice::waitOnFence(const Fence *fence) {
    }

    void OpenGLRenderingDevice::destroyFence(Fence *fence) {
    }

    Semaphore *OpenGLRenderingDevice::createSemaphore() {
    }

    void OpenGLRenderingDevice::destroySemaphore(Semaphore *semaphore) {
    }

    CommandPool *OpenGLRenderingDevice::createCommandPool(uint32_t queueFamily, CommandBufferType type) {
    }

    void OpenGLRenderingDevice::resetCommandPool(CommandPool *pool) {
    }

    void OpenGLRenderingDevice::destroyCommandPool(CommandPool *pool) {
    }

    CommandBuffer *OpenGLRenderingDevice::createCommandBuffer(CommandPool *pool) {
    }

    void OpenGLRenderingDevice::beginCommandBuffer(CommandBuffer *commandBuffer) {
    }

    void OpenGLRenderingDevice::endCommandBuffer(CommandBuffer *commandBuffer) {
    }

    Buffer *OpenGLRenderingDevice::createBuffer(BufferUsage usage, uint32_t count, uint32_t stride) {
    }

    void OpenGLRenderingDevice::destroyBuffer(Buffer *buffer) {
    }

    uint32_t OpenGLRenderingDevice::getQueueFamily(QueueFamilyFlags queueFamilyFlags, Surface *surface) {
    }

    CommandQueue *OpenGLRenderingDevice::createCommandQueue() {
    }

    void OpenGLRenderingDevice::executeCommandQueueAndPresent(CommandQueue *commandQueue,
                                                              const std::vector<Semaphore *> &waitSemaphores,
                                                              const std::vector<CommandBuffer *> &commandBuffers,
                                                              const std::vector<Semaphore *> &semaphores, Fence *fence,
                                                              const std::vector<Swapchain *> &swapchains) {
    }

    void OpenGLRenderingDevice::destroyCommandQueue(CommandQueue *commandQueue) {
    }

    Image *OpenGLRenderingDevice::createImage(const ImageFormat &format, const ImageView &view) {
    }

    std::byte *OpenGLRenderingDevice::mapImage(Image *image) {
    }

    void OpenGLRenderingDevice::unmapImage(Image *image) {
    }

    void OpenGLRenderingDevice::destroyImage(Image *image) {
    }

    Sampler *OpenGLRenderingDevice::createSampler(SamplerState state) {
    }

    void OpenGLRenderingDevice::destroySampler(Sampler *sampler) {
    }

    Shader *OpenGLRenderingDevice::createShaderFromSpirv(const std::string &name,
                                                         const std::vector<ShaderStageData> &stages) {
    }

    void OpenGLRenderingDevice::destroyShaderModules(Shader *shader) {
    }

    void OpenGLRenderingDevice::destroyShader(Shader *shader) {
    }
}
