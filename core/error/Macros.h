#pragma once

#include <string>

namespace Vixen {
    void log_error(std::string function, std::string file, int line, std::string message);
}

#define ASSERT_THROW(CONDITION, ERROR, MESSAGE) \
    if (!(CONDITION)) { \
        log_error(__FUNCTION__, __FILE__, __LINE__, "Assertion failed \"" #CONDITION "\"\n" MESSAGE); \
        throw ERROR(MESSAGE); \
    }

#ifdef DEBUG_ENABLED
#define DEBUG_ASSERT(CONDITION) \
    if (!(CONDITION)) { \
        log_error(__FUNCTION__, __FILE__, __LINE__, "Debug assertion failed \"" #CONDITION "\"."); \
        __builtin_trap(); \
    }
#else
#define DEBUG_ASSERT(CONDITION)
#endif
