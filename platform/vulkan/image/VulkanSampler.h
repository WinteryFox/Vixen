#pragma once

#include <memory>
#include <volk.h>

#include "core/image/Sampler.h"

namespace Vixen {
    class VulkanRenderingDevice;

    struct VulkanSampler final : Sampler {
        std::shared_ptr<VulkanRenderingDevice> device;

        VkSampler sampler;

        VulkanSampler(const SamplerState &state, VkSampler sampler)
            : Sampler(state),
              sampler(sampler) {
        }
    };
}
