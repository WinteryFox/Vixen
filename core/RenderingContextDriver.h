#pragma once

#include "DriverDevice.h"
#include "RenderingDeviceDriver.h"

namespace Vixen {
    struct Window;
    struct Surface;

    class RenderingContextDriver {
    public:
        RenderingContextDriver() = default;

        virtual ~RenderingContextDriver() = default;

        virtual std::vector<DriverDevice> getDevices() = 0;

        virtual bool deviceSupportsPresent(uint32_t deviceIndex, Surface* surface) = 0;

        virtual RenderingDeviceDriver *createRenderingDeviceDriver(uint32_t deviceIndex, uint32_t frameCount) = 0;

        virtual void destroyRenderingDeviceDriver(RenderingDeviceDriver *renderingDeviceDriver) = 0;

        virtual auto createSurface(Window *window) -> std::expected<Surface*, Error> = 0;

        virtual void destroySurface(Surface *surface) = 0;
    };
}
