#pragma once

#include "core/RenderingContext.h"

namespace Vixen {
    class OpenGLRenderingContext final : public RenderingContext {
    public:
        OpenGLRenderingContext();

        ~OpenGLRenderingContext() override;
    };
}
