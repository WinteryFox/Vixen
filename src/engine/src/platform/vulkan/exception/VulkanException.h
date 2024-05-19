#pragma once

#include <stdexcept>

namespace Vixen {
    class VulkanException final : public std::runtime_error {
    public:
        explicit VulkanException(const std::string &message)
            : runtime_error(message) {}

        explicit VulkanException(const char *message)
            : runtime_error(message) {}

        explicit VulkanException(runtime_error &&cause)
            : runtime_error(cause) {}

        explicit VulkanException(const runtime_error &cause)
            : runtime_error(cause) {}
    };
}
