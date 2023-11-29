#pragma once

#include "Device.h"

namespace Vixen::Vk {
    class VkSampler {
        std::shared_ptr<Device> device;

        ::VkSampler sampler;

    public:
        VkSampler(const std::shared_ptr<Device>& device, VkSamplerCreateInfo info);

        explicit VkSampler(const std::shared_ptr<Device>& device);

        VkSampler(VkSampler& other) = delete;

        VkSampler& operator=(const VkSampler& other) = delete;

        VkSampler(VkSampler&& other) noexcept;

        VkSampler const& operator=(VkSampler&& other) noexcept;

        ~VkSampler();

        [[nodiscard]] ::VkSampler getSampler() const;
    };
}
