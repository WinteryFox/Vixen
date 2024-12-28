#pragma once
#include "SamplerState.h"

namespace Vixen {
    class Sampler {
    protected:
        SamplerState state;

    public:
        explicit Sampler(const SamplerState &state);

        virtual ~Sampler() = default;
    };
}
