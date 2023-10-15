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
        R wait(uint32_t index, uint64_t timeout, const F &lambda) {
            auto &fence = fences[index];
            auto result = vkWaitForFences(device->getDevice(), 1, &fence, VK_TRUE, timeout);

            if (result == VK_TIMEOUT)
                spdlog::warn("Waiting on fence index {} timed out", index);
            else
                checkVulkanResult(result, "Fence was signaled with an error");

            return lambda(fence);
        }

        template<typename R, typename F>
        R waitFirst(uint64_t timeout, const F &lambda) {
            auto result = vkWaitForFences(device->getDevice(), fences.size(), fences.data(), VK_FALSE, timeout);

            if (result == VK_TIMEOUT)
                spdlog::warn("Waiting on fences timed out");
            else
                checkVulkanResult(result, "Fence was signaled with an error");

            for (size_t i = 0; i < fences.size(); i++) {
                auto &fence = fences[i];
                auto v = vkGetFenceStatus(device->getDevice(), fence);

                if (v == VK_SUCCESS)
                    return lambda(fence, i);
            }

            throw std::runtime_error("Ono");
        }

        void waitAll(uint64_t timeout);

        void resetAll();
    };
}
