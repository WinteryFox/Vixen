#pragma once

#include <volk.h>

namespace Vixen {
    enum class ImageSwizzle {
        Identity,
        Zero,
        One,
        Red,
        Green,
        Blue,
        Alpha
    };

    static_assert(ENUM_MEMBERS_EQUAL(ImageSwizzle::Identity, VK_COMPONENT_SWIZZLE_IDENTITY));
    static_assert(ENUM_MEMBERS_EQUAL(ImageSwizzle::Zero, VK_COMPONENT_SWIZZLE_ZERO));
    static_assert(ENUM_MEMBERS_EQUAL(ImageSwizzle::One, VK_COMPONENT_SWIZZLE_ONE));
    static_assert(ENUM_MEMBERS_EQUAL(ImageSwizzle::Red, VK_COMPONENT_SWIZZLE_R));
    static_assert(ENUM_MEMBERS_EQUAL(ImageSwizzle::Green, VK_COMPONENT_SWIZZLE_G));
    static_assert(ENUM_MEMBERS_EQUAL(ImageSwizzle::Blue, VK_COMPONENT_SWIZZLE_B));
    static_assert(ENUM_MEMBERS_EQUAL(ImageSwizzle::Alpha, VK_COMPONENT_SWIZZLE_A));
}
