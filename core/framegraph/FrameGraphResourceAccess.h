#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string_view>

#include "FrameGraphResourceAccessError.h"

namespace Vixen {
    /**
     * @brief Supplies names and indices used to contextualize permission failures.
     */
    struct FrameGraphResourceAccessContext {
        std::optional<std::uint32_t> passIndex = std::nullopt;
        std::string_view passName{};
        std::string_view resourceName{};
        bool sideEffecting = false;
        bool usesExternallySynchronizedResources = false;
    };

    /**
     * @brief Authorizes one exact logical resource-version operation.
     *
     * The permission table must be sorted by logical resource index and contain
     * no more than one permission per logical resource. Authorization never
     * resolves physical storage: versions that alias one physical allocation
     * remain distinct capability tokens.
     *
     * @param permissions Immutable permissions compiled for the current pass.
     * @param request The exact handle, type, access mode, and optional usage requested.
     * @param context Optional diagnostic names and indices.
     * @return The matching permission, or a contextual authorization error.
     */
    [[nodiscard]] auto authorizeFrameGraphResourceAccess(
        std::span<const FrameGraphResourcePermission> permissions,
        const FrameGraphResourceAccessRequest& request,
        FrameGraphResourceAccessContext context = {}
    ) -> std::expected<const FrameGraphResourcePermission*, FrameGraphResourceAccessError>;
}
