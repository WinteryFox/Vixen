#pragma once

#include "FrameGraphResources.h"
#include "command/CommandBuffer.h"

namespace Vixen {
    class RenderingDeviceDriver;

    struct RenderPassContext {
        RenderingDeviceDriver& driver;

        CommandBuffer* commandBuffer;

        const FrameGraphResources& resources;
    };
}
