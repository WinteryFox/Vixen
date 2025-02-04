#pragma once

namespace Vixen {
    enum class SamplerRepeatMode : uint32_t {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder,
        MirrorClampToEdge
    };
}
