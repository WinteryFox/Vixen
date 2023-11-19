#pragma once

#include "Device.h"

namespace Vixen::Vk {
    class VkDescriptorPool {
        std::shared_ptr<Device> device;

        ::VkDescriptorPool pool;

    public:
        VkDescriptorPool(
            const std::shared_ptr<Device>& device,
            const std::vector<VkDescriptorPoolSize>& sizes,
            uint32_t maxSets
        );

        VkDescriptorPool(const VkDescriptorPool& other) = delete;

        VkDescriptorPool(VkDescriptorPool&& other) noexcept = delete;

        VkDescriptorPool& operator=(VkDescriptorPool other) = delete;

        ~VkDescriptorPool();
    };
}
