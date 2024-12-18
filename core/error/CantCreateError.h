#pragma once

#include <stdexcept>

namespace Vixen {
    class CantCreateError final : public std::runtime_error {
    public:
        explicit CantCreateError(const std::string& message)
            : runtime_error(message) {}

        explicit CantCreateError(const char* message)
            : runtime_error(message) {}

        explicit CantCreateError(runtime_error&& cause)
            : runtime_error(cause) {}

        explicit CantCreateError(const runtime_error& cause)
            : runtime_error(cause) {}
    };
}
