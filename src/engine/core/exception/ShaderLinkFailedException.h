#pragma once
#include <stdexcept>

namespace Vixen {
    class ShaderLinkFailedException final : public std::runtime_error {
    public:
        explicit ShaderLinkFailedException(const std::string &message)
            : runtime_error(message) {}

        explicit ShaderLinkFailedException(const char *message)
            : runtime_error(message) {}

        explicit ShaderLinkFailedException(runtime_error &&cause)
            : runtime_error(cause) {}

        explicit ShaderLinkFailedException(const runtime_error &cause)
            : runtime_error(cause) {}
    };
}
