#pragma once

namespace Vixen {
    struct Window;
    class RenderingDevice;
    struct Surface;

    class RenderingContext {
    public:
        RenderingContext() = default;

        virtual ~RenderingContext() = default;

        virtual RenderingDevice *createDevice() = 0;

        virtual Surface *createSurface(Window *window) = 0;

        virtual void destroySurface(Surface *surface) = 0;
    };
}
