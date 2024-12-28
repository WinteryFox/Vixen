#pragma once

#include "CompareOperator.h"
#include "SamplerBorderColor.h"
#include "SamplerFilter.h"
#include "SamplerRepeatMode.h"

namespace Vixen {
    struct SamplerState {
        SamplerFilter mag;
        SamplerFilter min;
        SamplerFilter mip;
        SamplerRepeatMode u;
        SamplerRepeatMode v;
        SamplerRepeatMode w;
        float lodBias;
        bool useAnisotropy;
        float maxAnisotropy;
        bool enableCompare;
        CompareOperator compareOperator;
        float minLod;
        float maxLod;
        SamplerBorderColor borderColor;
        bool unnormalizedCoordinates;
    };
}
