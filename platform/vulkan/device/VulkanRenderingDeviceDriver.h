#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <mutex>
#include <string>
#include <vector>
#include <volk.h>

#include "DeviceFeatureSupport.h"
#include "core/rendering/RenderingDeviceDriver.h"
#include "core/image/ImageSamples.h"

typedef struct VmaAllocator_T* VmaAllocator;

namespace Vixen {
    struct ImageSubresourceLayers;
    struct VulkanCommandQueue;
    struct VulkanSwapchain;
    class VulkanRenderingContextDriver;

    class VulkanRenderingDeviceDriver final : public RenderingDeviceDriver {
        struct Features {
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
        DeviceFeatureSupport physicalDeviceFeatures;
        VkPhysicalDeviceProperties physicalDeviceProperties;
        VkDeviceSize maxBufferSize;

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

        auto validateImageFormatSupport(const VkImageCreateInfo& info) const
            -> std::expected<void, ResourceCreationError>;

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
        ) -> std::expected<void, CommandError> override;

        void destroyCommandPool(
            CommandPool* pool
        ) override;

        auto createCommandBuffer(
            CommandPool* pool
        ) -> std::expected<CommandBuffer*, ResourceCreationError> override;

        [[nodiscard]] auto beginCommandBuffer(
            CommandBuffer* commandBuffer
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto endCommandBuffer(
            CommandBuffer* commandBuffer
        ) -> std::expected<void, CommandError> override;

        auto createCommandQueue(
            uint32_t queueFamilyIndex
        ) -> std::expected<CommandQueue*, Error> override;

        auto executeCommandQueueAndPresent(
            CommandQueue* commandQueue,
            const std::vector<Semaphore*>& waitSemaphores,
            const std::vector<CommandBuffer*>& commandBuffers,
            const std::vector<Semaphore*>& signalSemaphores,
            Fence* fence,
            const std::vector<Swapchain*>& swapchains
        ) -> std::expected<void, CommandError> override;

        void destroyCommandQueue(
            CommandQueue* commandQueue
        ) override;

        auto createBuffer(
            uint64_t size,
            BufferUsageFlags usage,
            MemoryAllocationType memoryType
        ) -> std::expected<Buffer*, ResourceCreationError> override;

        void destroyBuffer(
            Buffer* buffer
        ) override;

        auto createImage(
            const ImageFormat& format,
            const ImageView& view
        ) -> std::expected<Image*, ResourceCreationError> override;

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

        auto createPipelineLayout(
            const PipelineLayoutDescription& description
        ) -> std::expected<PipelineLayout*, ResourceCreationError> override;

        void destroyPipelineLayout(PipelineLayout* pipelineLayout) override;

        auto createGraphicsPipeline(
            const GraphicsPipelineDescription& description
        ) -> std::expected<GraphicsPipeline*, ResourceCreationError> override;

        auto createComputePipeline(
            const ComputePipelineDescription& description
        ) -> std::expected<ComputePipeline*, ResourceCreationError> override;

        void destroyPipeline(Pipeline* pipeline) override;

        static VkImageSubresourceLayers _imageSubresourceLayers(
            const ImageSubresourceLayers& layers
        );

        static VkBufferImageCopy _bufferImageCopyRegion(
            const BufferImageCopyRegion& region
        );

        [[nodiscard]] auto commandBeginRenderPass(
            CommandBuffer* commandBuffer,
            const RenderingInfo& renderingInfo
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandEndRenderPass(
            CommandBuffer* commandBuffer
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandSetViewport(
            CommandBuffer* commandBuffer,
            const std::vector<glm::uvec2>& viewports
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandSetScissor(
            CommandBuffer* commandBuffer,
            const std::vector<glm::uvec2>& scissors
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandSetBlendConstants(
            CommandBuffer* commandBuffer,
            glm::vec4 blendConstants
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandBindVertexBuffers(
            CommandBuffer* commandBuffer,
            const std::vector<const Buffer*>& buffers,
            const std::vector<uint64_t>& offsets
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandBindIndexBuffers(
            CommandBuffer* commandBuffer,
            const Buffer* buffer,
            IndexFormat format,
            uint64_t offset
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandPushConstants(
            CommandBuffer* commandBuffer,
            const PipelineLayout* pipelineLayout,
            ShaderStageFlags stages,
            uint32_t offset,
            std::span<const std::byte> data
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandBindGraphicsPipeline(
            CommandBuffer* commandBuffer,
            const GraphicsPipeline* pipeline
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandBindComputePipeline(
            CommandBuffer* commandBuffer,
            const ComputePipeline* pipeline
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandDraw(
            CommandBuffer* commandBuffer,
            uint32_t vertexCount,
            uint32_t instanceCount,
            uint32_t firstVertex,
            uint32_t firstInstance
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandDrawIndexed(
            CommandBuffer* commandBuffer,
            uint32_t indexCount,
            uint32_t instanceCount,
            uint32_t firstIndex,
            int32_t vertexOffset,
            uint32_t firstInstance
        ) -> std::expected<void, CommandError> override;

        auto commandDispatch(
            CommandBuffer* commandBuffer,
            uint32_t groupCountX,
            uint32_t groupCountY,
            uint32_t groupCountZ
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandPipelineBarrier(
            CommandBuffer* commandBuffer,
            PipelineStageFlags sourceStages,
            PipelineStageFlags destinationStages,
            const std::vector<MemoryBarrier>& memoryBarriers,
            const std::vector<BufferBarrier>& bufferBarriers,
            const std::vector<ImageBarrier>& imageBarriers
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandClearBuffer(
            CommandBuffer* commandBuffer,
            Buffer* buffer,
            uint64_t offset,
            uint64_t size
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandCopyBuffer(
            CommandBuffer* commandBuffer,
            Buffer* source,
            Buffer* destination,
            const std::vector<BufferCopyRegion>& regions
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandCopyImage(
            CommandBuffer* commandBuffer,
            Image* source,
            ImageLayout sourceLayout,
            Image* destination,
            ImageLayout destinationLayout,
            const std::vector<ImageCopyRegion>& regions
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandResolveImage(
            CommandBuffer* commandBuffer,
            Image* source,
            ImageLayout sourceLayout,
            uint32_t sourceLayer,
            uint32_t sourceMipmap,
            Image* destination,
            ImageLayout destinationLayout,
            uint32_t destinationLayer,
            uint32_t destinationMipmap
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandClearColorImage(
            CommandBuffer* commandBuffer,
            Image* image,
            ImageLayout imageLayout,
            const glm::vec4& color,
            const ImageSubresourceRange& subresource
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandCopyBufferToImage(
            CommandBuffer* commandBuffer,
            Buffer* buffer,
            Image* image,
            ImageLayout layout,
            const std::vector<BufferImageCopyRegion>& regions
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandCopyImageToBuffer(
            CommandBuffer* commandBuffer,
            Image* image,
            ImageLayout layout,
            Buffer* buffer,
            const std::vector<BufferImageCopyRegion>& regions
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandBeginLabel(
            CommandBuffer* commandBuffer,
            const std::string& label,
            const glm::vec4& color
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto commandEndLabel(
            CommandBuffer* commandBuffer
        ) -> std::expected<void, CommandError> override;

        [[nodiscard]] auto getImageUsageSupportedByFormat(
            ImageDataFormat format,
            bool isCpuReadable
        ) const -> std::expected<ImageUsageFlags, ResourceCreationError> override;

        [[nodiscard]] auto getTexelBufferUsageSupportedByFormat(
            ImageDataFormat format
        ) const -> std::expected<BufferUsageFlags, ResourceCreationError> override;

        [[nodiscard]] auto validateAttachmentFormatSupport(
            ImageDataFormat format,
            ImageUsageBits usage,
            ImageSamples samples
        ) const -> std::expected<void, ResourceCreationError> override;

        [[nodiscard]] PipelineLayoutLimits getPipelineLayoutLimits() const override;

        [[nodiscard]] uint64_t getMaxBufferSize() const override;

        [[nodiscard]] uint32_t getMaxTexelBufferElements() const override;

        [[nodiscard]] uint32_t getMaxColorAttachments() const override;

        [[nodiscard]] uint32_t getMaxVertexInputBindings() const override;

        [[nodiscard]] uint32_t getMaxVertexInputAttributes() const override;

        [[nodiscard]] uint32_t getMaxVertexInputBindingStride() const override;

        [[nodiscard]] uint32_t getMaxVertexInputAttributeOffset() const override;

        [[nodiscard]] bool isVertexInputFormatSupported(
            ImageDataFormat format
        ) const override;

        [[nodiscard]] bool isColorBlendSupported(
            ImageDataFormat format
        ) const override;
    };
}
