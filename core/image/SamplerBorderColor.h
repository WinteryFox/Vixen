#pragma once

#include <volk.h>

#include "../Bitmask.h"

namespace Vixen {
    enum class SamplerBorderColor {
        FloatTransparentBlack,
        IntTransparentBlack,
        FloatOpaqueBlack,
        IntOpaqueBlack,
        FloatOpaqueWhite,
        IntOpaqueWhite
    };

    static_assert(ENUM_MEMBERS_EQUAL(SamplerBorderColor::FloatTransparentBlack, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerBorderColor::IntTransparentBlack, VK_BORDER_COLOR_INT_TRANSPARENT_BLACK));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerBorderColor::FloatOpaqueBlack, VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerBorderColor::IntOpaqueBlack, VK_BORDER_COLOR_INT_OPAQUE_BLACK));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerBorderColor::FloatOpaqueWhite, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerBorderColor::IntOpaqueWhite, VK_BORDER_COLOR_INT_OPAQUE_WHITE));
}
