#pragma once

#include "Vulkan.h"

namespace Vixen::Vk {
    class VkFence {
        ::VkDevice device;

        ::VkFence fence;

    public:
        VkFence(::VkDevice device, bool createSignaled);

        VkFence(const VkFence &) = delete;

        VkFence(VkFence &&o) noexcept;

        VkFence &operator=(const VkFence &) = delete;

        ~VkFence();

        template<typename R, typename F>
        R wait(uint64_t timeout, const F &lambda) {
            auto result = vkWaitForFences(device, 1, &fence, VK_TRUE, timeout);

            if (result == VK_TIMEOUT)
                spdlog::warn("Waiting on fence timed out");
            else
                checkVulkanResult(result, "Fence was signaled with an error");

            return lambda(fence);
        }

        void reset();
    };
}
