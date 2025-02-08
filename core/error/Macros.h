#pragma once

#include <source_location>
#include <string>
#include <spdlog/spdlog.h>

namespace Vixen {
    template<typename T>
    constexpr void error(const std::string &message,
                         const std::source_location location = std::source_location::current()) {
        spdlog::error("ERROR: {}\n    {} ({}:{})", message, location.function_name(), location.file_name(),
                      location.line());
        throw T(message);
    }
}

#ifdef DEBUG_ENABLED
#define DEBUG_ASSERT(CONDITION) \
    if (!(CONDITION)) { \
        error<CantCreateError>("Debug assertion failed"); \
        __builtin_trap(); \
    }
#else
#define DEBUG_ASSERT(CONDITION)
#endif
