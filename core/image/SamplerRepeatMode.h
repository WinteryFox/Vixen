#pragma once

#include <volk.h>

#include "../Bitmask.h"

namespace Vixen {
    enum class SamplerRepeatMode : uint32_t {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder,
        MirrorClampToEdge
    };

    static_assert(ENUM_MEMBERS_EQUAL(SamplerRepeatMode::Repeat, VK_SAMPLER_ADDRESS_MODE_REPEAT));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerRepeatMode::MirroredRepeat, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerRepeatMode::ClampToEdge, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerRepeatMode::ClampToBorder, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER));
    static_assert(ENUM_MEMBERS_EQUAL(SamplerRepeatMode::MirrorClampToEdge, VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE));
}
