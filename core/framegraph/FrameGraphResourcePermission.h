#pragma once

#include <optional>
#include <variant>

#include "Resource.h"

namespace Vixen {
    /**
     * @brief Identifies whether a resource access belongs to a render attachment.
     *
     * Attachment accesses use the same permission model as ordinary image
     * accesses. The role is retained so that permission failures can explain
     * how the resource was requested and attachment-specific rules can be
     * enforced without maintaining a second authorization path.
     */
    enum class FrameGraphAttachmentRole {
        None,
        Color,
        DepthStencil
    };

    /**
     * @brief Stores the resource-type-specific usage required by an access.
     */
    using FrameGraphResourceUsageKind = std::variant<
        ImageUsageBits,
        BufferUsageBits
    >;

    /**
     * @brief Describes one resource access requested while executing a pass.
     *
     * Usage is optional because some accessors may only ask for read or write
     * authority. When present, it allows the permission matcher to require an
     * exact image or buffer usage as well.
     */
    struct FrameGraphResourceAccessRequest {
        ResourceId handle;
        ResourceType type;
        ResourceAccess access;

        std::optional<FrameGraphResourceUsageKind> usage = std::nullopt;
        PipelineStageFlags stages{};
        FrameGraphAttachmentRole attachmentRole = FrameGraphAttachmentRole::None;

        friend bool operator==(
            const FrameGraphResourceAccessRequest&,
            const FrameGraphResourceAccessRequest&
        ) = default;
    };

    /**
     * @brief Normalized runtime authority granted to one pass for one logical resource.
     *
     * Read declarations use only input, write declarations use only output, and
     * read-write declarations use both. An unused endpoint contains an invalid
     * ResourceId. Keeping both endpoints is necessary because a write advances
     * the logical resource version while a read-write access consumes the input
     * version and produces the output version.
     */
    struct FrameGraphResourcePermission {
        ResourceId input;
        ResourceId output;

        ResourceType type;
        ResourceAccess access;
        FrameGraphResourceUsageKind usage;
        PipelineStageFlags stages;

        FrameGraphAttachmentRole attachmentRole = FrameGraphAttachmentRole::None;

        friend bool operator==(
            const FrameGraphResourcePermission&,
            const FrameGraphResourcePermission&
        ) = default;
    };
}
