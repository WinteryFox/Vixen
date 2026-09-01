#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "FrameGraphResourcePermission.h"

namespace Vixen {
    /**
     * @brief Classifies failures while authorizing or resolving a pass resource access.
     */
    enum class FrameGraphResourceAccessErrorCode {
        InvalidHandle,
        UndeclaredResource,
        UndeclaredVersion,
        ResourceTypeMismatch,
        AccessDenied,
        UsageMismatch,
        RangeOutOfBounds,
        ViewMismatch,
        UnresolvedResource,
        InvalidInvariant
    };

    /**
     * @brief Describes a failed runtime resource-access request.
     *
     * requestedOperation records what the executing pass attempted.
     * declaredOperations contains the relevant permissions that were compiled
     * for that pass; it is empty when no matching declaration exists. Together
     * these fields allow diagnostics to report the exact access, version,
     * usage, stages, and attachment role involved in a failure.
     */
    struct FrameGraphResourceAccessError {
        FrameGraphResourceAccessErrorCode code;
        std::string message;

        std::optional<std::uint32_t> passIndex = std::nullopt;
        std::optional<std::uint32_t> resourceIndex = std::nullopt;
        std::optional<std::uint32_t> resourceVersion = std::nullopt;

        std::optional<std::string> passName = std::nullopt;
        std::optional<std::string> resourceName = std::nullopt;

        std::optional<FrameGraphResourceAccessRequest> requestedOperation = std::nullopt;
        std::vector<FrameGraphResourcePermission> declaredOperations{};
        std::vector<std::string> details{};
    };
}
