#pragma once

#include <cstdint>
#include <expected>
#include <map>
#include <vector>

#include "DriverDevice.h"
#include "Frame.h"
#include "core/image/ImageDataFormat.h"
#include "core/memory/MemoryAllocationType.h"
#include "core/buffer/BufferUsage.h"
#include "core/error/Error.h"
#include "core/error/ResourceCreationError.h"
#include "core/image/ImageFormat.h"
#include "core/image/ImageView.h"

namespace Vixen {
    struct ComputePipelineDescription;
    class PipelineLayout;
    struct PipelineLayoutDescription;
    struct GraphicsPipelineDescription;
    struct ComputePipeline;
    struct GraphicsPipeline;
    struct Framebuffer;
    class RenderingContextDriver;
    class RenderingDeviceDriver;
    struct Window;
    struct CommandQueue;
    class Buffer;
    struct Image;

    class RenderingDevice {
        RenderingContextDriver* renderingContextDriver;
        RenderingDeviceDriver* renderingDeviceDriver;

        DriverDevice device;

        uint32_t graphicsQueueFamily;
        uint32_t transferQueueFamily;
        CommandQueue* graphicsQueue;
        CommandQueue* transferQueue;

        uint32_t frameIndex;
        std::vector<Frame> frames;
        uint64_t framesDrawn;

        std::map<Window*, Swapchain*> swapchains;

        void waitForFrame(
            uint32_t frameIndex
        );

        void waitForFrames();

        void drainDeferredReleases(Frame& frame);

        void flushAndWaitForFrames();

        void beginFrame(
            bool presented
        );

        void endFrame();

        void executeChainedCommands(
            bool present,
            Fence* drawFence,
            Semaphore* drawSemaphoreToSignal
        );

        void executeFrame(
            bool present
        );

        [[nodiscard]] auto createTexelBuffer(
            uint32_t elementCount,
            ImageDataFormat format,
            BufferUsageBits usage
        ) const -> std::expected<Buffer*, ResourceCreationError>;

    public:
        RenderingDevice(
            RenderingContextDriver* renderingContext,
            Window* mainWindow
        );

        ~RenderingDevice();

        void swapBuffers(
            bool present
        );

        void submit();

        void sync();

        void deferRelease(DeferredRelease release);

        void deferDestroy(Image* image);

        void deferDestroy(Buffer* buffer);

        auto createScreen(
            Window* window
        ) -> std::expected<Swapchain*, Error>;

        auto prepareScreenForDrawing(
            Window* window
        ) -> std::expected<Framebuffer*, Error>;

        void destroyScreen(
            Window* window
        );

        [[nodiscard]] auto createBuffer(
            uint64_t size,
            BufferUsageFlags usage,
            MemoryAllocationType memoryType
        ) const -> std::expected<Buffer*, ResourceCreationError>;

        [[nodiscard]] auto createVertexBuffer(uint64_t size) const -> std::expected<Buffer*, ResourceCreationError>;

        [[nodiscard]] auto createUniformBuffer(uint64_t size) const -> std::expected<Buffer*, ResourceCreationError>;

        [[nodiscard]] auto createStorageBuffer(uint64_t size) const -> std::expected<Buffer*, ResourceCreationError>;

        [[nodiscard]] auto createUniformTexelBuffer(
            uint32_t elementCount,
            ImageDataFormat format
        ) const -> std::expected<Buffer*, ResourceCreationError>;

        [[nodiscard]] auto createStorageTexelBuffer(
            uint32_t elementCount,
            ImageDataFormat format
        ) const -> std::expected<Buffer*, ResourceCreationError>;

        [[nodiscard]] auto createImage(
            const ImageFormat& format,
            const ImageView& view
        ) const -> std::expected<Image*, ResourceCreationError>;

        [[nodiscard]]
        auto createPipelineLayout(
            const PipelineLayoutDescription& description
        ) const -> std::expected<PipelineLayout*, ResourceCreationError>;

        [[nodiscard]]
        auto createGraphicsPipeline(
            const GraphicsPipelineDescription& description
        ) const -> std::expected<GraphicsPipeline*, ResourceCreationError>;

        [[nodiscard]]
        auto createComputePipeline(
            const ComputePipelineDescription& description
        ) const -> std::expected<ComputePipeline*, ResourceCreationError>;

        [[nodiscard]] RenderingContextDriver* getRenderingContextDriver() const;

        [[nodiscard]] RenderingDeviceDriver* getRenderingDeviceDriver() const;
    };
}
