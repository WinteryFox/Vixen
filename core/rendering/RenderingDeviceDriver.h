#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "command/CommandError.h"
#include "core/synchronization/BufferBarrier.h"
#include "core/synchronization/ImageBarrier.h"
#include "core/memory/MemoryAllocationType.h"
#include "core/synchronization/MemoryBarrier.h"
#include "core/synchronization/PipelineStageFlags.h"
#include "core/command/QueueFamilyFlags.h"
#include "core/buffer/BufferUsage.h"
#include "core/error/ResourceCreationError.h"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"
#include "core/image/ImageFormat.h"
#include "core/image/ImageView.h"
#include "core/image/SamplerState.h"
#include "core/shader/ShaderStageData.h"
#include "pipeline/ComputePipelineDescription.h"

namespace Vixen {
    struct DescriptorCountLimits {
        uint32_t samplers;
        uint32_t uniformBuffers;
        uint32_t storageBuffers;
        uint32_t sampledImages;
        uint32_t storageImages;
        uint32_t inputAttachments;
    };

    struct PipelineLayoutLimits {
        uint32_t maxBoundDescriptorSets;
        DescriptorCountLimits maxDescriptors;
        DescriptorCountLimits maxPerStageDescriptors;
        uint32_t maxPerStageResources;
    };

    struct GraphicsPipelineDescription;
    struct ComputePipeline;
    struct GraphicsPipeline;
    struct PipelineLayoutDescription;
    class Pipeline;
    class PipelineLayout;
    struct ShaderReflectionError;
    struct BufferImageCopyRegion;
    struct ImageSubresourceRange;
    struct ImageCopyRegion;
    enum class ImageLayout;
    struct BufferCopyRegion;
    enum class IndexFormat;
    struct RenderingInfo;
    enum class ShaderLanguage;
    struct Sampler;
    struct SamplerState;
    struct Image;
    class Buffer;
    class CommandBuffer;
    struct CommandPool;
    enum class CommandBufferType;
    struct Semaphore;
    class Fence;
    enum class SwapchainError;
    enum class Error;
    class Shader;
    struct Surface;
    class Swapchain;
    struct CommandQueue;
    struct Framebuffer;

    class RenderingDeviceDriver {
        enum class RenderingScope { Any, Outside, Inside };

        static auto checkRecording(
            const CommandBuffer* commandBuffer,
            std::string_view operation,
            QueueFamilyFlags allowedQueues = {},
            RenderingScope scope = RenderingScope::Any
        ) -> std::expected<void, CommandError>;

        static auto checkGraphicsDrawState(
            const CommandBuffer* commandBuffer,
            std::string_view operation
        ) -> std::expected<void, CommandError>;

        static auto checkVertexBindings(
            const CommandBuffer* commandBuffer,
            std::string_view operation,
            uint32_t count,
            uint32_t instanceCount,
            std::optional<uint32_t> firstVertex,
            uint32_t firstInstance
        ) -> std::expected<void, CommandError>;

    protected:
        static auto reflectShader(
            const std::vector<ShaderStageData>& stages,
            Shader* shader
        ) -> std::expected<void, ShaderReflectionError>;

    public:
        virtual ~RenderingDeviceDriver() = default;

        virtual auto createSwapchain(
            Surface* surface
        ) -> std::expected<Swapchain*, Error> = 0;

        virtual auto resizeSwapchain(
            CommandQueue* commandQueue,
            Swapchain* swapchain,
            uint32_t imageCount
        ) -> std::expected<void, Error> = 0;

        virtual auto acquireSwapchainFramebuffer(
            CommandQueue* commandQueue,
            Swapchain* swapchain
        ) -> std::expected<Framebuffer*, SwapchainError> = 0;

        virtual void destroySwapchain(
            Swapchain* swapchain
        ) = 0;

        virtual auto createFence() -> std::expected<Fence*, Error> = 0;

        virtual auto waitOnFence(
            Fence* fence
        ) -> std::expected<void, Error> = 0;

        virtual void destroyFence(
            Fence* fence
        ) = 0;

        virtual auto createSemaphore() -> std::expected<Semaphore*, Error> = 0;

        virtual void destroySemaphore(
            Semaphore* semaphore
        ) = 0;

        virtual auto createCommandPool(
            uint32_t queueFamily,
            CommandBufferType type
        ) -> std::expected<CommandPool*, Error> = 0;

        virtual auto resetCommandPool(
            CommandPool* pool
        ) -> std::expected<void, CommandError> = 0;

        virtual void destroyCommandPool(
            CommandPool* pool
        ) = 0;

        virtual auto createCommandBuffer(
            CommandPool* pool
        ) -> std::expected<CommandBuffer*, ResourceCreationError> = 0;

        [[nodiscard]] virtual auto beginCommandBuffer(
            CommandBuffer* commandBuffer
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto endCommandBuffer(
            CommandBuffer* commandBuffer
        ) -> std::expected<void, CommandError> = 0;

        virtual auto createBuffer(
            uint64_t size,
            BufferUsageFlags usage,
            MemoryAllocationType memoryType
        ) -> std::expected<Buffer*, ResourceCreationError> = 0;

        virtual void destroyBuffer(
            Buffer* buffer
        ) = 0;

        virtual auto getQueueFamily(
            QueueFamilyFlags queueFamilyFlags,
            Surface* surface
        ) -> std::expected<uint32_t, Error> = 0;

        virtual auto createCommandQueue(
            uint32_t queueFamilyIndex
        ) -> std::expected<CommandQueue*, Error> = 0;

        virtual auto executeCommandQueueAndPresent(
            CommandQueue* commandQueue,
            const std::vector<Semaphore*>& waitSemaphores,
            const std::vector<CommandBuffer*>& commandBuffers,
            const std::vector<Semaphore*>& signalSemaphores,
            Fence* fence,
            const std::vector<Swapchain*>& swapchains
        ) -> std::expected<void, CommandError> = 0;

        virtual void destroyCommandQueue(
            CommandQueue* commandQueue
        ) = 0;

        virtual auto createImage(
            const ImageFormat& format,
            const ImageView& view
        ) -> std::expected<Image*, ResourceCreationError> = 0;

        virtual std::byte* mapImage(
            Image* image
        ) = 0;

        virtual void unmapImage(
            Image* image
        ) = 0;

        virtual void destroyImage(
            Image* image
        ) = 0;

        virtual auto createSampler(
            SamplerState state
        ) -> std::expected<Sampler*, Error> = 0;

        virtual void destroySampler(
            Sampler* sampler
        ) = 0;

        virtual std::vector<std::byte> compileSpirvFromSource(
            ShaderStageBits stage,
            const std::string& source,
            ShaderLanguage language
        );

        virtual Shader* createShaderFromSpirv(
            const std::string& name,
            const std::vector<ShaderStageData>& stages
        ) = 0;

        virtual void destroyShaderModules(
            Shader* shader
        ) = 0;

        virtual void destroyShader(
            Shader* shader
        ) = 0;

        virtual auto createPipelineLayout(
            const PipelineLayoutDescription& description
        ) -> std::expected<PipelineLayout*, ResourceCreationError> = 0;

        virtual void destroyPipelineLayout(
            PipelineLayout* pipelineLayout
        ) = 0;

        virtual auto createGraphicsPipeline(
            const GraphicsPipelineDescription& description
        ) -> std::expected<GraphicsPipeline*, ResourceCreationError> = 0;

        virtual auto createComputePipeline(
            const ComputePipelineDescription& description
        ) -> std::expected<ComputePipeline*, ResourceCreationError> = 0;

        virtual void destroyPipeline(Pipeline* pipeline) = 0;

        [[nodiscard]] virtual auto commandBeginRenderPass(
            CommandBuffer* commandBuffer,
            const RenderingInfo& renderingInfo
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandEndRenderPass(
            CommandBuffer* commandBuffer
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandSetViewport(
            CommandBuffer* commandBuffer,
            const std::vector<glm::uvec2>& viewports
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandSetScissor(
            CommandBuffer* commandBuffer,
            const std::vector<glm::uvec2>& scissors
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandSetBlendConstants(
            CommandBuffer* commandBuffer,
            glm::vec4 blendConstants
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandBindVertexBuffers(
            CommandBuffer* commandBuffer,
            const std::vector<const Buffer*>& buffers,
            const std::vector<uint64_t>& offsets
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandBindIndexBuffers(
            CommandBuffer* commandBuffer,
            const Buffer* buffer,
            IndexFormat format,
            uint64_t offset
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandDraw(
            CommandBuffer* commandBuffer,
            uint32_t vertexCount,
            uint32_t instanceCount,
            uint32_t firstVertex,
            uint32_t firstInstance
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandDrawIndexed(
            CommandBuffer* commandBuffer,
            uint32_t indexCount,
            uint32_t instanceCount,
            uint32_t firstIndex,
            int32_t vertexOffset,
            uint32_t firstInstance
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandDispatch(
            CommandBuffer* commandBuffer,
            uint32_t groupCountX,
            uint32_t groupCountY,
            uint32_t groupCountZ
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandPipelineBarrier(
            CommandBuffer* commandBuffer,
            PipelineStageFlags sourceStages,
            PipelineStageFlags destinationStages,
            const std::vector<MemoryBarrier>& memoryBarriers,
            const std::vector<BufferBarrier>& bufferBarriers,
            const std::vector<ImageBarrier>& imageBarriers
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandClearBuffer(
            CommandBuffer* commandBuffer,
            Buffer* buffer,
            uint64_t offset,
            uint64_t size
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandCopyBuffer(
            CommandBuffer* commandBuffer,
            Buffer* source,
            Buffer* destination,
            const std::vector<BufferCopyRegion>& regions
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandCopyImage(
            CommandBuffer* commandBuffer,
            Image* source,
            ImageLayout sourceLayout,
            Image* destination,
            ImageLayout destinationLayout,
            const std::vector<ImageCopyRegion>& regions
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandResolveImage(
            CommandBuffer* commandBuffer,
            Image* source,
            ImageLayout sourceLayout,
            uint32_t sourceLayer,
            uint32_t sourceMipmap,
            Image* destination,
            ImageLayout destinationLayout,
            uint32_t destinationLayer,
            uint32_t destinationMipmap
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandClearColorImage(
            CommandBuffer* commandBuffer,
            Image* image,
            ImageLayout imageLayout,
            const glm::vec4& color,
            const ImageSubresourceRange& subresource
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandCopyBufferToImage(
            CommandBuffer* commandBuffer,
            Buffer* buffer,
            Image* image,
            ImageLayout layout,
            const std::vector<BufferImageCopyRegion>& regions
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandCopyImageToBuffer(
            CommandBuffer* commandBuffer,
            Image* image,
            ImageLayout layout,
            Buffer* buffer,
            const std::vector<BufferImageCopyRegion>& regions
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandBeginLabel(
            CommandBuffer* commandBuffer,
            const std::string& label,
            const glm::vec4& color
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto commandEndLabel(
            CommandBuffer* commandBuffer
        ) -> std::expected<void, CommandError> = 0;

        [[nodiscard]] virtual auto getImageUsageSupportedByFormat(
            ImageDataFormat format,
            bool isCpuReadable
        ) const -> std::expected<ImageUsageFlags, ResourceCreationError> = 0;

        [[nodiscard]] virtual auto getTexelBufferUsageSupportedByFormat(
            ImageDataFormat format
        ) const -> std::expected<BufferUsageFlags, ResourceCreationError> = 0;

        [[nodiscard]] virtual auto validateAttachmentFormatSupport(
            ImageDataFormat format,
            ImageUsageBits usage,
            ImageSamples samples
        ) const -> std::expected<void, ResourceCreationError> = 0;

        [[nodiscard]] virtual PipelineLayoutLimits getPipelineLayoutLimits() const = 0;

        [[nodiscard]] virtual uint64_t getMaxBufferSize() const = 0;

        [[nodiscard]] virtual uint32_t getMaxTexelBufferElements() const = 0;

        [[nodiscard]] virtual uint32_t getMaxColorAttachments() const = 0;

        [[nodiscard]] virtual uint32_t getMaxVertexInputBindings() const = 0;

        [[nodiscard]] virtual uint32_t getMaxVertexInputAttributes() const = 0;

        [[nodiscard]] virtual uint32_t getMaxVertexInputBindingStride() const = 0;

        [[nodiscard]] virtual uint32_t getMaxVertexInputAttributeOffset() const = 0;

        [[nodiscard]] virtual bool isVertexInputFormatSupported(
            ImageDataFormat format
        ) const = 0;

        [[nodiscard]] virtual bool isColorBlendSupported(
            ImageDataFormat format
        ) const = 0;
    };
}
