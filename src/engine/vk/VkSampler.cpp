#include "VkSampler.h"

namespace Vixen::Vk {
    VkSampler::VkSampler(const std::shared_ptr<Device>& device, VkSamplerCreateInfo info)
        : device(device),
          sampler(VK_NULL_HANDLE) {
        checkVulkanResult(
            vkCreateSampler(device->getDevice(), &info, nullptr, &sampler),
            "Failed to create sampler"
        );
    }

    VkSampler::VkSampler(const std::shared_ptr<Device>& device) : VkSampler(
        device,
        {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 1,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE
        }
    ) {}

    VkSampler::VkSampler(VkSampler&& other) noexcept
        : device(std::exchange(other.device, nullptr)),
          sampler(std::exchange(other.sampler, nullptr)) {}

    VkSampler const& VkSampler::operator=(VkSampler&& other) noexcept {
        std::swap(device, other.device);
        std::swap(sampler, other.sampler);

        return *this;
    }

    VkSampler::~VkSampler() {
        vkDestroySampler(device->getDevice(), sampler, nullptr);
    }

    ::VkSampler VkSampler::getSampler() const {
        return sampler;
    }
}
