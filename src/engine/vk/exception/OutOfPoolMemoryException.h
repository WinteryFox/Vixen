#pragma once
#include <stdexcept>

namespace Vixen::Vk {
    class OutOfPoolMemoryException final : public std::runtime_error {
    public:
        explicit OutOfPoolMemoryException(const std::string& message)
            : runtime_error(message) {}

        explicit OutOfPoolMemoryException(const char* message)
            : runtime_error(message) {}

        explicit OutOfPoolMemoryException(runtime_error&& cause)
            : runtime_error(cause) {}

        explicit OutOfPoolMemoryException(const runtime_error& cause)
            : runtime_error(cause) {}
    };
}
