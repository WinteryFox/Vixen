#pragma once

#include <map>

#include "DriverDevice.h"
#include "RenderingDeviceDriver.h"

namespace Vixen {
    struct Window;
    struct Surface;

    class RenderingContextDriver {
        std::map<Window*, Surface*> surfaces;

    public:
        RenderingContextDriver() = default;

        virtual ~RenderingContextDriver() = default;

        virtual std::vector<DriverDevice> getDevices() = 0;

        virtual bool deviceSupportsPresent(uint32_t deviceIndex, Surface* surface) = 0;

        virtual RenderingDeviceDriver* createRenderingDeviceDriver(uint32_t deviceIndex, uint32_t frameCount) = 0;

        virtual void destroyRenderingDeviceDriver(RenderingDeviceDriver* renderingDeviceDriver) = 0;

        Surface* getSurfaceFromWindow(Window* window) const;

        auto createWindow(Window* window) -> std::expected<void, Error>;

        void setWindowSize(Window* window, uint32_t width, uint32_t height);

        void setWindowVSyncMode(Window* window, VSyncMode vsyncMode);

        void destroyWindow(Window* window);

        virtual auto createSurface(Window* window) -> std::expected<Surface*, Error> = 0;

        virtual bool getSurfaceNeedsResize(Surface* surface) = 0;

        virtual void setSurfaceNeedsResize(Surface* surface, bool needsResize) = 0;

        virtual void setSurfaceSize(Surface* surface, uint32_t width, uint32_t height) = 0;

        virtual void setSurfaceVSyncMode(Surface* surface, VSyncMode vsyncMode) = 0;

        virtual void destroySurface(Surface* surface) = 0;
    };
}
