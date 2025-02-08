#pragma once

#include <stdexcept>

namespace Vixen {
    class CantCreateError final : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };
}
