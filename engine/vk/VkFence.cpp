#include "VkFence.h"

namespace Vixen::Vk {
    VkFence::VkFence(const std::shared_ptr<Device> &device, uint32_t count, bool createSignaled)
            : device(device) {
        fences.resize(count);

        VkFenceCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = static_cast<VkFenceCreateFlags>(createSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0)
        };

        for (size_t i = 0; i < fences.size(); i++)
            vkCreateFence(device->getDevice(), &info, nullptr, fences.data() + i);
    }

    VkFence::~VkFence() {
        for (const auto &fence: fences)
            vkDestroyFence(device->getDevice(), fence, nullptr);
    }

    void VkFence::waitAll(uint64_t timeout) {
        vkWaitForFences(device->getDevice(), fences.size(), fences.data(), VK_TRUE, timeout);
    }

    void VkFence::resetAll() {
        vkResetFences(device->getDevice(), fences.size(), fences.data());
    }
}
