#pragma once

#include <expected>

#include "FrameGraphError.h"
#include "Resource.h"

namespace Vixen {
    [[nodiscard]] auto mapImageResourceState(
        ResourceAccess access,
        ImageUsageBits usage,
        PipelineStageFlags stages
    ) -> std::expected<ImageState, FrameGraphError>;

    [[nodiscard]] auto mapBufferResourceState(
        ResourceAccess access,
        BufferUsageBits usage,
        PipelineStageFlags stages
    ) -> std::expected<BufferState, FrameGraphError>;

    [[nodiscard]] auto validateImageUsage(
        ImageUsageFlags declared,
        ImageUsageBits requested
    ) -> std::expected<void, FrameGraphError>;

    [[nodiscard]] auto validateBufferUsage(
        BufferUsageFlags declared,
        BufferUsageBits requested
    ) -> std::expected<void, FrameGraphError>;
}
