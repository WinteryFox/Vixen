#pragma once

#include <cstdint>

#define DECLARE_BITMASK(enumName) \
inline enumName operator|(const enumName lhs, const enumName rhs) { \
    return static_cast<enumName>(static_cast<int64_t>(lhs) | static_cast<int64_t>(rhs)); \
} \
\
inline bool operator&(const enumName lhs, const enumName rhs) { \
    return (static_cast<int64_t>(lhs) & static_cast<int64_t>(rhs)); \
}

#define ENUM_MEMBERS_EQUAL(a, b) ((int64_t) a == (int64_t) b)
