#pragma once

namespace Vixen {
    class FrameGraphResources;
    struct CommandBuffer;
    class RenderingDeviceDriver;

    struct RenderPassContext {
        RenderingDeviceDriver& driver;

        CommandBuffer* commandBuffer;

        const FrameGraphResources& resources;
    };
}
