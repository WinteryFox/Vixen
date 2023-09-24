#pragma once

#include <Device.h>

namespace Vixen::Vk {
    class VkFence {
        std::shared_ptr<Device> device;

        std::vector<::VkFence> fences;

    public:
        VkFence(const std::shared_ptr<Device> &device, uint32_t count, bool createSignaled);

        VkFence(const VkFence &) = delete;

        ~VkFence();

        template<typename R, typename F>
        R waitFirst(uint64_t timeout, bool reset, const F &lambda) {
            vkWaitForFences(device->getDevice(), fences.size(), fences.data(), VK_FALSE, timeout);

            for (const auto &fence: fences) {
                auto v = vkGetFenceStatus(device->getDevice(), fence);

                if (v == VK_SUCCESS) {
                    if (reset)
                        vkResetFences(device->getDevice(), 1, &fence);

                    return lambda(fence);
                } else if (v == VK_TIMEOUT)
                    spdlog::debug("Fence timed out");
                else
                    checkVulkanResult(v, "Fence was signaled with an error");
            }

            throw std::runtime_error("Ono");
        }

        void waitAll(uint64_t timeout);

        void resetAll();
    };
}
