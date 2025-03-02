#pragma once

#include <vector>

#include "Frame.h"
#include "RenderingContextDriver.h"
#include "Window.h"
#include "command/CommandQueue.h"

namespace Vixen {
    class RenderingDevice {
        RenderingContextDriver *renderingContextDriver;
        RenderingDeviceDriver *renderingDeviceDriver;

        DriverDevice device;

        uint32_t graphicsQueueFamily;
        uint32_t transferQueueFamily;
        uint32_t presentQueueFamily;
        CommandQueue *graphicsQueue;
        CommandQueue *transferQueue;
        CommandQueue *presentQueue;

        uint32_t frame;
        std::vector<Frame> frames;
        uint64_t framesDrawn;

        void beginFrame();

        void endFrame();

        void executeFrame(bool present);

    public:
        RenderingDevice(RenderingContextDriver *renderingContext, Window *window);

        ~RenderingDevice();

        void swapBuffers(bool present);

        void submit();

        void sync();

        [[nodiscard]] RenderingContextDriver *getRenderingContextDriver() const;

        [[nodiscard]] RenderingDeviceDriver *getRenderingDeviceDriver() const;
    };
}
