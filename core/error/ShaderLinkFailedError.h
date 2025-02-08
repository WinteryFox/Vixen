#pragma once
#include <stdexcept>

namespace Vixen {
    class ShaderLinkFailedError final : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };
}
