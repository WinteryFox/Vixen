#pragma once

#include <vector>
#include <volk.h>

#include "core/RenderingDevice.h"
#include "GraphicsCard.h"

typedef struct VmaAllocator_T *VmaAllocator;

namespace Vixen {
    struct VulkanSwapchain;
    class VulkanRenderingContext;

    class VulkanRenderingDevice final : public RenderingDevice {
        struct Features {
            bool dynamicRendering;
            bool synchronization2;
            bool deviceFault;
        } enabledFeatures;

        struct Queue {
            VkQueue queue = VK_NULL_HANDLE;
            uint32_t count = 0;
        };

        VulkanRenderingContext *renderingContext;

        GraphicsCard physicalDevice;

        std::vector<std::string> enabledExtensionNames;

        VkDevice device;

        std::vector<std::vector<Queue> > queueFamilies;

        VmaAllocator allocator;

        void initializeExtensions();

        void checkFeatures() const;

        void checkCapabilities();

        void initializeDevice();

        [[nodiscard]] VkSampleCountFlagBits findClosestSupportedSampleCount(const ImageSamples &samples) const;

    public:
        VulkanRenderingDevice(VulkanRenderingContext *renderingContext, uint32_t deviceIndex);

        VulkanRenderingDevice(const VulkanRenderingDevice &) = delete;

        VulkanRenderingDevice &operator=(const VulkanRenderingDevice &) = delete;

        VulkanRenderingDevice(VulkanRenderingDevice &&) = delete;

        VulkanRenderingDevice &operator=(VulkanRenderingDevice &&) = delete;

        ~VulkanRenderingDevice() override;

        Swapchain *createSwapchain(Surface *surface) override;

        void resizeSwapchain(CommandQueue *commandQueue, Swapchain *swapchain, uint32_t imageCount) override;

        Framebuffer *acquireSwapchainFramebuffer(CommandQueue *commandQueue, Swapchain *swapchain,
                                                 bool &resizeRequired) override;

    private:
        void _destroySwapchain(const VulkanSwapchain *swapchain);

    public:
        void destroySwapchain(Swapchain *swapchain) override;

        uint32_t getQueueFamily(QueueFamilyFlags queueFamilyFlags, Surface *surface) override;

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

        CommandQueue *createCommandQueue() override;

        void executeCommandQueueAndPresent(CommandQueue *commandQueue,
                                           const std::vector<Semaphore *> &waitSemaphores,
                                           const std::vector<CommandBuffer *> &commandBuffers,
                                           const std::vector<Semaphore *> &semaphores,
                                           Fence *fence, const std::vector<Swapchain *> &swapchains) override;

        void destroyCommandQueue(CommandQueue *commandQueue) override;

        Buffer *createBuffer(BufferUsage usage, uint32_t count, uint32_t stride) override;

        void destroyBuffer(Buffer *buffer) override;

        Image *createImage(const ImageFormat &format, const ImageView &view) override;

        std::byte *mapImage(Image *image) override;

        void unmapImage(Image *image) override;

        void destroyImage(Image *image) override;

        Sampler *createSampler(SamplerState state) override;

        void destroySampler(Sampler *sampler) override;

        Shader *createShaderFromSpirv(const std::string &name, const std::vector<ShaderStageData> &stages) override;

        void destroyShaderModules(Shader *shader) override;

        void destroyShader(Shader *shader) override;

        [[nodiscard]] GraphicsCard getPhysicalDevice() const;

        static VkImageSubresourceLayers _imageSubresourceLayers(const ImageSubresourceLayers &layers);

        static VkBufferImageCopy _bufferImageCopyRegion(const BufferImageCopyRegion &region);

        void commandPipelineBarrier(CommandBuffer *commandBuffer,
                                    PipelineStageFlags sourceStages,
                                    PipelineStageFlags destinationStages,
                                    const std::vector<MemoryBarrier> &memoryBarriers,
                                    const std::vector<BufferBarrier> &bufferBarriers,
                                    const std::vector<ImageBarrier> &imageBarriers) override;

        void commandClearBuffer(CommandBuffer *commandBuffer, Buffer *buffer, uint64_t offset, uint64_t size) override;

        void commandCopyBuffer(CommandBuffer *commandBuffer, Buffer *source, Buffer *destination,
                               const std::vector<BufferCopyRegion> &regions) override;

        void commandCopyImage(CommandBuffer *commandBuffer, Image *source, ImageLayout sourceLayout, Image *destination,
                              ImageLayout destinationLayout, const std::vector<ImageCopyRegion> &regions) override;

        void commandResolveImage(CommandBuffer *commandBuffer, Image *source, ImageLayout sourceLayout,
                                 uint32_t sourceLayer, uint32_t sourceMipmap, Image *destination,
                                 ImageLayout destinationLayout,
                                 uint32_t destinationLayer, uint32_t destinationMipmap) override;

        void commandClearColorImage(CommandBuffer *commandBuffer, Image *image, ImageLayout imageLayout,
                                    const glm::vec4 &color, const ImageSubresourceRange &subresource) override;

        void commandCopyBufferToImage(CommandBuffer *commandBuffer, Buffer *buffer, Image *image, ImageLayout layout,
                                      const std::vector<BufferImageCopyRegion> &regions) override;

        void commandCopyImageToBuffer(CommandBuffer *commandBuffer, Image *image, ImageLayout layout, Buffer *buffer,
                                      const std::vector<BufferImageCopyRegion> &regions) override;

        void commandBeginLabel(CommandBuffer *commandBuffer, const std::string &label, const glm::vec3 &color) override;

        void commandEndLabel(CommandBuffer *commandBuffer) override;
    };
}
