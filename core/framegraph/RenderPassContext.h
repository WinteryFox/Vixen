#pragma once

#include "FrameGraphPassResources.h"

namespace Vixen {
    struct CommandBuffer;
    class RenderingDeviceDriver;

    /**
     * @brief Noncopyable execution context valid only for the current pass callback.
     */
    struct RenderPassContext final {
        RenderingDeviceDriver& driver;

        CommandBuffer* commandBuffer;

        FrameGraphPassResources& resources;

        RenderPassContext(
            RenderingDeviceDriver& driver,
            CommandBuffer* commandBuffer,
            FrameGraphPassResources& resources
        ) noexcept : driver(driver),
                     commandBuffer(commandBuffer),
                     resources(resources) {}

        RenderPassContext(const RenderPassContext&) = delete;
        RenderPassContext& operator=(const RenderPassContext&) = delete;
        RenderPassContext(RenderPassContext&&) = delete;
        RenderPassContext& operator=(RenderPassContext&&) = delete;
        ~RenderPassContext() = default;
    };
}
