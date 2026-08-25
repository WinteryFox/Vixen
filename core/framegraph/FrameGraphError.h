#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

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
        ResourceTypeMismatch
    };

    struct FrameGraphError {
        FrameGraphErrorCode code;
        std::string message;

        std::optional<uint32_t> passIndex{};
        std::optional<uint32_t> resourceIndex{};
        std::optional<uint32_t> resourceVersion{};

        std::optional<std::string> passName{};
        std::optional<std::string> resourceName{};

        std::vector<std::string> details{};
    };
}
