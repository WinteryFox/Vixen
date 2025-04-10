#pragma once

#include <mutex>
#include <vector>
#include <volk.h>

#include "core/RenderingDeviceDriver.h"

typedef struct VmaAllocator_T* VmaAllocator;

namespace Vixen {
    struct VulkanCommandQueue;
    struct VulkanSwapchain;
    class VulkanRenderingContextDriver;

    class VulkanRenderingDeviceDriver final : public RenderingDeviceDriver {
        struct Features {
            bool dynamicRendering;
            bool synchronization2;
            bool deviceFault;
        } enabledFeatures;

        struct Queue {
            VkQueue queue = VK_NULL_HANDLE;
            uint32_t virtualCount = 0;
            std::mutex submitMutex{};
        };

        VulkanRenderingContextDriver* renderingContext;

        uint32_t deviceIndex;
        VkPhysicalDevice physicalDevice;
        VkPhysicalDeviceFeatures physicalDeviceFeatures;
        VkPhysicalDeviceProperties physicalDeviceProperties;

        std::vector<std::string> enabledExtensionNames;

        VkDevice device;

        std::vector<std::vector<Queue>> queueFamilies;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;

        VmaAllocator allocator;

        uint32_t frameCount;

        auto initializeExtensions() -> std::expected<void, Error>;

        void checkFeatures() const;

        void checkCapabilities();

        auto initializeDevice() -> std::expected<void, Error>;

        [[nodiscard]] VkSampleCountFlagBits findClosestSupportedSampleCount(
            const ImageSamples& samples
        ) const;

    public:
        VulkanRenderingDeviceDriver(
            VulkanRenderingContextDriver* renderingContext,
            uint32_t deviceIndex,
            uint32_t frameCount
        );

        VulkanRenderingDeviceDriver(const VulkanRenderingDeviceDriver&) = delete;

        VulkanRenderingDeviceDriver& operator=(const VulkanRenderingDeviceDriver&) = delete;

        VulkanRenderingDeviceDriver(VulkanRenderingDeviceDriver&&) = delete;

        VulkanRenderingDeviceDriver& operator=(VulkanRenderingDeviceDriver&&) = delete;

        ~VulkanRenderingDeviceDriver() override;

        auto createSwapchain(
            Surface* surface
        ) -> std::expected<Swapchain*, Error> override;

        auto resizeSwapchain(
            CommandQueue* commandQueue,
            Swapchain* swapchain,
            uint32_t imageCount
        ) -> std::expected<void, Error> override;

        auto acquireSwapchainFramebuffer(
            CommandQueue* commandQueue,
            Swapchain* swapchain
        ) -> std::expected<Framebuffer*, SwapchainError> override;

    private:
        void releaseSwapchain(
            VulkanSwapchain* swapchain
        );

        static auto releaseImageSemaphore(
            VulkanCommandQueue* commandQueue,
            uint32_t semaphoreIndex,
            bool releaseOnSwapchain
        ) -> std::expected<void, Error>;

        auto recreateImageSemaphore(
            VulkanCommandQueue* commandQueue,
            uint32_t semaphoreIndex,
            bool releaseOnSwapchain
        ) const -> std::expected<void, Error>;

    public:
        void destroySwapchain(
            Swapchain* swapchain
        ) override;

        auto getQueueFamily(
            QueueFamilyFlags queueFamilyFlags,
            Surface* surface
        ) -> std::expected<uint32_t, Error> override;

        auto createFence() -> std::expected<Fence*, Error> override;

        auto waitOnFence(
            Fence* fence
        ) -> std::expected<void, Error> override;

        void destroyFence(
            Fence* fence
        ) override;

        auto createSemaphore() -> std::expected<Semaphore*, Error> override;

        void destroySemaphore(
            Semaphore* semaphore
        ) override;

        auto createCommandPool(
            uint32_t queueFamily,
            CommandBufferType type
        ) -> std::expected<CommandPool*, Error> override;

        auto resetCommandPool(
            CommandPool* pool
        ) -> std::expected<void, Error> override;

        void destroyCommandPool(
            CommandPool* pool
        ) override;

        auto createCommandBuffer(
            CommandPool* pool
        ) -> std::expected<CommandBuffer*, Error> override;

        auto beginCommandBuffer(
            CommandBuffer* commandBuffer
        ) -> std::expected<void, Error> override;

        void endCommandBuffer(
            CommandBuffer* commandBuffer
        ) override;

        auto createCommandQueue(
            uint32_t queueFamilyIndex
        ) -> std::expected<CommandQueue*, Error> override;

        auto executeCommandQueueAndPresent(
            CommandQueue* commandQueue,
            const std::vector<Semaphore*>& waitSemaphores,
            const std::vector<CommandBuffer*>& commandBuffers,
            const std::vector<Semaphore*>& semaphores,
            Fence* fence,
            const std::vector<Swapchain*>& swapchains
        ) -> std::expected<void, Error> override;

        void destroyCommandQueue(
            CommandQueue* commandQueue
        ) override;

        auto createBuffer(
            BufferUsage usage,
            uint32_t count,
            uint32_t stride
        ) -> std::expected<Buffer*, Error> override;

        void destroyBuffer(
            Buffer* buffer
        ) override;

        auto createImage(
            const ImageFormat& format,
            const ImageView& view
        ) -> std::expected<Image*, Error> override;

        std::byte* mapImage(
            Image* image
        ) override;

        void unmapImage(
            Image* image
        ) override;

        void destroyImage(
            Image* image
        ) override;

        auto createSampler(
            SamplerState state
        ) -> std::expected<Sampler*, Error> override;

        void destroySampler(
            Sampler* sampler
        ) override;

        Shader* createShaderFromSpirv(
            const std::string& name,
            const std::vector<ShaderStageData>& stages
        ) override;

        void destroyShaderModules(
            Shader* shader
        ) override;

        void destroyShader(
            Shader* shader
        ) override;

        static VkImageSubresourceLayers _imageSubresourceLayers(
            const ImageSubresourceLayers& layers
        );

        static VkBufferImageCopy _bufferImageCopyRegion(
            const BufferImageCopyRegion& region
        );

        void commandBeginRenderPass(
            CommandBuffer* commandBuffer,
            RenderPass* renderPass,
            Framebuffer* framebuffer,
            CommandBufferType commandBufferType,
            const glm::uvec2& rectangle,
            const std::vector<ClearValue>& clearValues
        ) override;

        void commandEndRenderPass(
            CommandBuffer* commandBuffer
        ) override;

        void commandSetViewport(
            CommandBuffer* commandBuffer,
            const std::vector<glm::uvec2>& viewports
        ) override;

        void commandSetScissor(
            CommandBuffer* commandBuffer,
            const std::vector<glm::uvec2>& scissors
        ) override;

        void commandBindVertexBuffers(
            CommandBuffer* commandBuffer,
            uint32_t count,
            const std::vector<Buffer*>& buffers,
            const std::vector<uint64_t>& offsets
        ) override;

        void commandBindIndexBuffers(
            CommandBuffer* commandBuffer,
            Buffer* buffer,
            IndexFormat format,
            uint64_t offset
        ) override;

        void commandPipelineBarrier(
            CommandBuffer* commandBuffer,
            PipelineStageFlags sourceStages,
            PipelineStageFlags destinationStages,
            const std::vector<MemoryBarrier>& memoryBarriers,
            const std::vector<BufferBarrier>& bufferBarriers,
            const std::vector<ImageBarrier>& imageBarriers
        ) override;

        void commandClearBuffer(
            CommandBuffer* commandBuffer,
            Buffer* buffer,
            uint64_t offset,
            uint64_t size
        ) override;

        void commandCopyBuffer(
            CommandBuffer* commandBuffer,
            Buffer* source,
            Buffer* destination,
            const std::vector<BufferCopyRegion>& regions
        ) override;

        void commandCopyImage(
            CommandBuffer* commandBuffer,
            Image* source,
            ImageLayout sourceLayout,
            Image* destination,
            ImageLayout destinationLayout,
            const std::vector<ImageCopyRegion>& regions
        ) override;

        void commandResolveImage(
            CommandBuffer* commandBuffer,
            Image* source,
            ImageLayout sourceLayout,
            uint32_t sourceLayer,
            uint32_t sourceMipmap,
            Image* destination,
            ImageLayout destinationLayout,
            uint32_t destinationLayer,
            uint32_t destinationMipmap
        ) override;

        void commandClearColorImage(
            CommandBuffer* commandBuffer,
            Image* image,
            ImageLayout imageLayout,
            const glm::vec4& color,
            const ImageSubresourceRange& subresource
        ) override;

        void commandCopyBufferToImage(
            CommandBuffer* commandBuffer,
            Buffer* buffer,
            Image* image,
            ImageLayout layout,
            const std::vector<BufferImageCopyRegion>& regions
        ) override;

        void commandCopyImageToBuffer(
            CommandBuffer* commandBuffer,
            Image* image,
            ImageLayout layout,
            Buffer* buffer,
            const std::vector<BufferImageCopyRegion>& regions
        ) override;

        void commandBeginLabel(
            CommandBuffer* commandBuffer,
            const std::string& label,
            const glm::vec3& color
        ) override;

        void commandEndLabel(
            CommandBuffer* commandBuffer
        ) override;
    };
}
