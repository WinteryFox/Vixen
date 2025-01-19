#pragma once

#include "Window.h"

namespace Vixen {
    struct Surface;

    class RenderingContext {
    public:
        RenderingContext() = default;

        virtual ~RenderingContext() = default;

        virtual Surface *createSurface(Window *window) = 0;

        virtual void destroySurface(Surface *surface) = 0;
    };
}
