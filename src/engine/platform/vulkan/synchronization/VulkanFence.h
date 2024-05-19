#pragma once

#include "../Vulkan.h"

namespace Vixen {
    class VulkanDevice;

    class VulkanFence {
        std::shared_ptr<VulkanDevice> device;

        ::VkFence fence;

    public:
        VulkanFence(const std::shared_ptr<VulkanDevice>& device, bool createSignaled);

        VulkanFence(const VulkanFence&) = delete;

        VulkanFence& operator=(const VulkanFence&) = delete;

        VulkanFence(VulkanFence&& other) noexcept;

        VulkanFence& operator=(VulkanFence&& other) noexcept;

        ~VulkanFence();

        void wait(uint64_t timeout = std::numeric_limits<uint64_t>::max()) const;

        void reset() const;

        [[nodiscard]] ::VkFence getFence() const;
    };
}
