#pragma once

#include "SamplerState.h"

namespace Vixen {
    struct Sampler {
        SamplerState state;

        virtual ~Sampler() = default;
    };
}
