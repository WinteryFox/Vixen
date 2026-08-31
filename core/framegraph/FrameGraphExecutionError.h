#pragma once

#include <cstdint>
#include <exception>
#include <optional>
#include <string>

namespace Vixen {
    enum class FrameGraphExecutionErrorCode {
        MovedFromGraph,
        InvalidCommandBuffer,
        CallbackFailed
    };

    struct FrameGraphExecutionError {
        FrameGraphExecutionErrorCode code;
        std::string message;

        std::optional<std::uint32_t> passIndex = std::nullopt;
        std::optional<std::string> passName = std::nullopt;

        std::exception_ptr cause = nullptr;
        bool commandBufferMustBeDiscarded = false;
    };
}
