#pragma once

#include <stdexcept>

namespace Vixen {
    class OutOfMemoryError final : public std::runtime_error {
    public:
        explicit OutOfMemoryError(const std::string& message)
            : runtime_error(message) {}

        explicit OutOfMemoryError(const char* message)
            : runtime_error(message) {}

        explicit OutOfMemoryError(runtime_error&& cause)
            : runtime_error(cause) {}

        explicit OutOfMemoryError(const runtime_error& cause)
            : runtime_error(cause) {}
    };
}
