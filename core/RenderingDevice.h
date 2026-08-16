#pragma once

#include <cstdint>
#include <expected>
#include <map>
#include <vector>

#include "DriverDevice.h"
#include "Frame.h"
#include "error/Error.h"

namespace Vixen {
    struct Framebuffer;
    class RenderingContextDriver;
    class RenderingDeviceDriver;
    class Window;
    struct CommandQueue;

    class RenderingDevice {
        RenderingContextDriver* renderingContextDriver;
        RenderingDeviceDriver* renderingDeviceDriver;

        DriverDevice device;

        uint32_t graphicsQueueFamily;
        uint32_t transferQueueFamily;
        uint32_t presentQueueFamily;
        CommandQueue* graphicsQueue;
        CommandQueue* transferQueue;
        CommandQueue* presentQueue;

        uint32_t frameIndex;
        std::vector<Frame> frames;
        uint64_t framesDrawn;

        std::map<Window*, Swapchain*> swapchains;

        void waitForFrame(
            uint32_t frameIndex
        );

        void waitForFrames();

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

        auto createScreen(
            Window* window
        ) -> std::expected<Swapchain*, Error>;

        auto prepareScreenForDrawing(
            Window* window
        ) -> std::expected<Framebuffer*, Error>;

        void destroyScreen(
            Window* window
        );

        [[nodiscard]] RenderingContextDriver* getRenderingContextDriver() const;

        [[nodiscard]] RenderingDeviceDriver* getRenderingDeviceDriver() const;
    };
}
