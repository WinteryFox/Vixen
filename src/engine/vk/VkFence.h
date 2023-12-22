#pragma once

#include "Vulkan.h"

namespace Vixen::Vk {
    class Device;

    class VkFence {
        std::shared_ptr<Device> device;

        ::VkFence fence;

    public:
        VkFence(const std::shared_ptr<Device>& device, bool createSignaled);

        VkFence(const VkFence&) = delete;

        VkFence& operator=(const VkFence&) = delete;

        VkFence(VkFence&& other) noexcept;

        VkFence& operator=(VkFence&& other) noexcept;

        ~VkFence();

        void wait(uint64_t timeout = std::numeric_limits<uint64_t>::max()) const;

        void reset() const;

        [[nodiscard]] ::VkFence getFence() const;
    };
}
