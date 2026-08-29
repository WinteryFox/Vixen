#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "error/ResourceCreationError.h"

namespace Vixen {
    enum class FrameGraphErrorCode {
        UnsupportedUsage,
        InvalidAccess,
        MissingPipelineStages,
        IncompatiblePipelineStages,
        UsageNotDeclared,
        ResourceVersionOverflow,
        DuplicateProducer,
        InvalidGraphInvariant,
        IncompatiblePassStages,
        InvalidResourceDeclaration,
        InvalidResourceOwnership,
        InvalidUsageShape,
        InvalidResourceHandle,
        ResourceTypeMismatch,
        InvalidResourceVersion,
        UninitializedResourceRead,
        MissingProducer,
        DependencyCycle,
        InvalidAttachment,
        ResourceAllocationFailed,
        UnresolvedResource
    };

    struct FrameGraphError {
        FrameGraphErrorCode code;
        std::string message;

        std::optional<uint32_t> passIndex = std::nullopt;
        std::optional<uint32_t> resourceIndex = std::nullopt;
        std::optional<uint32_t> resourceVersion = std::nullopt;

        std::optional<std::string> passName = std::nullopt;
        std::optional<std::string> resourceName = std::nullopt;

        std::optional<ResourceCreationError> cause = std::nullopt;
        std::vector<std::string> details{};
    };
}
