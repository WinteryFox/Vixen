#pragma once

#include <string>

namespace Vixen {
    enum class CommandErrorCode {
        InvalidState,
        InvalidArgument,
        OutOfHostMemory,
        OutOfDeviceMemory,
        NativeOperationFailed
    };

    struct CommandError {
        CommandErrorCode code;
        std::string message;
    };
}
