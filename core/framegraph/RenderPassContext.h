#pragma once

#include "FrameGraphResourceView.h"

namespace Vixen {
    struct CommandBuffer;
    class RenderingDeviceDriver;

    struct RenderPassContext {
        RenderingDeviceDriver& driver;

        CommandBuffer* commandBuffer;

        FrameGraphResourceView resources;
    };
}
