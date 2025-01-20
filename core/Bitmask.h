#pragma once

#include <cstdint>

#define DECLARE_BITMASK(ENUM_NAME) \
inline ENUM_NAME operator|(const ENUM_NAME lhs, const ENUM_NAME rhs) { \
    return static_cast<ENUM_NAME>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)); \
} \
\
inline ENUM_NAME operator|(const ENUM_NAME lhs, const uint32_t rhs) { \
    return static_cast<ENUM_NAME>(static_cast<uint32_t>(lhs) | rhs); \
} \
\
inline bool operator&(const ENUM_NAME lhs, const ENUM_NAME rhs) { \
    return static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs); \
} \
\
inline bool operator&(const ENUM_NAME lhs, const uint32_t rhs) { \
    return static_cast<uint32_t>(lhs) & (rhs); \
} \
\
inline bool operator==(const ENUM_NAME lhs, uint32_t rhs) { \
    return static_cast<uint32_t>(lhs) == (rhs); \
}

#define ENUM_MEMBERS_EQUAL(a, b) ((uint32_t) a == (uint32_t) b)
