#pragma once

#include "FrameGraphResources.h"
#include "RenderingDeviceDriver.h"
#include "command/CommandBuffer.h"

namespace Vixen {
    struct RenderPassContext {
        RenderingDeviceDriver& driver;

        CommandBuffer* commandBuffer;

        const FrameGraphResources& resources;
    };
}
