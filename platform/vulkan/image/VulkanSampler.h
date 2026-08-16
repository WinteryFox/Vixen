#pragma once

#include <volk.h>

#include "core/image/Sampler.h"

namespace Vixen {
    struct VulkanSampler final : Sampler {
        VkSampler sampler;
    };
}
