#include "VkFence.h"

namespace Vixen::Vk {
    VkFence::VkFence(::VkDevice device, uint32_t count, bool createSignaled)
            : device(device) {
        fences.resize(count);

        VkFenceCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = static_cast<VkFenceCreateFlags>(createSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0)
        };

        for (size_t i = 0; i < fences.size(); i++)
            vkCreateFence(device, &info, nullptr, fences.data() + i);
    }

    VkFence::VkFence(VkFence &&o) noexcept:
            device(std::exchange(o.device, VK_NULL_HANDLE)),
            fences(std::exchange(o.fences, {})) {}

    VkFence::~VkFence() {
        vkWaitForFences(
                device,
                fences.size(),
                fences.data(),
                VK_TRUE,
                std::numeric_limits<uint64_t>::max()
        );

        for (const auto &fence: fences)
            vkDestroyFence(device, fence, nullptr);
    }

    void VkFence::waitAll(uint64_t timeout) {
        vkWaitForFences(device, fences.size(), fences.data(), VK_TRUE, timeout);
    }

    void VkFence::resetAll() {
        vkResetFences(device, fences.size(), fences.data());
    }
}
