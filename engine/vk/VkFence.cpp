#include "VkFence.h"

namespace Vixen::Vk {
    VkFence::VkFence(::VkDevice device, bool createSignaled)
            : device(device) {
        VkFenceCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = static_cast<VkFenceCreateFlags>(createSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0)
        };

        vkCreateFence(device, &info, nullptr, &fence);
    }

    VkFence::VkFence(VkFence &&o) noexcept:
            device(std::exchange(o.device, VK_NULL_HANDLE)),
            fence(std::exchange(o.fence, VK_NULL_HANDLE)) {}

    VkFence::~VkFence() {
        wait<void>(
                std::numeric_limits<uint64_t>::max(),
                [this](const auto &f) {
                    vkDestroyFence(device, f, nullptr);
                }
        );
    }

    void VkFence::reset() {
        vkResetFences(device, 1, &fence);
    }

    const ::VkFence VkFence::getFence() const {
        return fence;
    }
}
