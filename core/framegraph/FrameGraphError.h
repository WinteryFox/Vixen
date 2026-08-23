#pragma once

#include <string>

namespace Vixen {
    enum class FrameGraphErrorCode {
        UnsupportedUsage,
        InvalidAccess,
        MissingPipelineStages,
        IncompatiblePipelineStages,
        UsageNotDeclared
    };

    struct FrameGraphError {
        FrameGraphErrorCode code;
        std::string message;
    };
}
