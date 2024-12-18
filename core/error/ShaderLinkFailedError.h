#pragma once
#include <stdexcept>

namespace Vixen {
    class ShaderLinkFailedError final : public std::runtime_error {
    public:
        explicit ShaderLinkFailedError(const std::string &message)
            : runtime_error(message) {}

        explicit ShaderLinkFailedError(const char *message)
            : runtime_error(message) {}

        explicit ShaderLinkFailedError(runtime_error &&cause)
            : runtime_error(cause) {}

        explicit ShaderLinkFailedError(const runtime_error &cause)
            : runtime_error(cause) {}
    };
}
