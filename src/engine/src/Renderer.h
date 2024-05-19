#pragma once

#include <vector>
#include "RenderPass.h"

namespace Vixen {
    class Renderer {
        // sort or bucket visible objects
        // foreach (render target)      -- framebuffer
        // foreach (pass)               -- depth, blending, etc. states
        // foreach (material)           -- shaders
        // foreach (material instance)  -- textures
        // foreach (vertex format)      -- vertex buffers, ideally we should pack multiple objects into
        //                              -- the same (dynamic or static) vertex/index buffer
        // foreach (object)
        //     writeUniformData(object)
        //     draw() -- take advantage of glDraw*() params to index into buffer without changing bindings
        //            -- draw commands must be batched (glMultiDrawElementsIndirect),
        //            -- batches can be written to in a parallel fashion, then kick with 1 API call

    public:
        virtual void submit() = 0;
    };
}
