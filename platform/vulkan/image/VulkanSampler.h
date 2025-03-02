#pragma once

#include <volk.h>

#include "core/image/Sampler.h"

namespace Vixen {
    class VulkanRenderingDeviceDriver;

    struct VulkanSampler final : Sampler {
        VkSampler sampler;
    };
}
