#pragma once

#include <string>

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
        IncompatiblePassStages
    };

    struct FrameGraphError {
        FrameGraphErrorCode code;
        std::string message;
    };
}
