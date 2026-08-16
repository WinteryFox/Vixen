#pragma once

#include <type_traits>

namespace Vixen {
    template <typename T>
    struct EnableFlags : std::false_type {};

    template <typename T>
    inline constexpr bool EnableFlagsV = EnableFlags<T>::value;

    template <typename T>
    concept FlagBit = std::is_enum_v<T> && EnableFlagsV<T>;

    template <typename Bit>
    class Flags {
        std::underlying_type_t<Bit> mask = 0;

    public:
        constexpr Flags() = default;

        constexpr Flags(Bit bit) : mask(static_cast<std::underlying_type_t<Bit>>(bit)) {}

        constexpr explicit Flags(std::underlying_type_t<Bit> mask) : mask(mask) {}

        [[nodiscard]]
        constexpr std::underlying_type_t<Bit> value() const {
            return mask;
        }

        [[nodiscard]]
        constexpr bool contains(Bit bit) const {
            return (mask & static_cast<std::underlying_type_t<Bit>>(bit)) != 0;
        }

        [[nodiscard]]
        constexpr bool empty() const {
            return mask == 0;
        }

        constexpr Flags& operator|=(Bit bit) {
            mask |= static_cast<std::underlying_type_t<Bit>>(bit);
            return *this;
        }

        constexpr Flags& operator|=(Flags other) {
            mask |= other.mask;
            return *this;
        }

        friend constexpr Flags operator|(Flags lhs, Flags rhs) { return Flags(lhs.mask | rhs.mask); }

        friend constexpr Flags operator&(Flags lhs, Flags rhs) { return Flags(lhs.mask & rhs.mask); }

        friend constexpr bool operator==(Flags lhs, Flags rhs) { return lhs.mask == rhs.mask; }
    };

    template <FlagBit Bit>
    constexpr Flags<Bit> operator|(Bit lhs, Bit rhs) {
        return Flags<Bit>(lhs) | Flags<Bit>(rhs);
    }

    template <FlagBit Bit>
    constexpr Flags<Bit> operator|(Flags<Bit> lhs, Bit rhs) {
        return lhs | Flags<Bit>(rhs);
    }

    template <FlagBit Bit>
    constexpr Flags<Bit> operator|(Bit lhs, Flags<Bit> rhs) {
        return Flags<Bit>(lhs) | rhs;
    }
}
