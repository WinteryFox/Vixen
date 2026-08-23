#pragma once

namespace Vixen {
    class FrameGraphResourceView;
    struct CommandBuffer;
    class RenderingDeviceDriver;

    struct RenderPassContext {
        RenderingDeviceDriver& driver;

        CommandBuffer* commandBuffer;

        const FrameGraphResourceView& resources;
    };
}
