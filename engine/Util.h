#pragma once

#include <string>
#include <stdexcept>

namespace Vixen {
    template<class T = std::runtime_error, typename... Args>
    inline void error(fmt::format_string<Args...> fmt, Args &&... args) {
        std::string m = fmt::format(fmt, std::forward<Args>(args)...);
        spdlog::error(m);
        throw T(m);
    }
}
