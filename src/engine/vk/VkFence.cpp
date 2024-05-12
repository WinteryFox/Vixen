#include "VkFence.h"

#include "Device.h"

namespace Vixen::Vk {
    VkFence::VkFence(const std::shared_ptr<Device> &device, const bool createSignaled)
        : device(device),
          fence(VK_NULL_HANDLE) {
        const VkFenceCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = static_cast<VkFenceCreateFlags>(createSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0)
        };

        vkCreateFence(device->getDevice(), &info, nullptr, &fence);
    }

    VkFence::VkFence(VkFence &&other) noexcept: device(std::exchange(other.device, VK_NULL_HANDLE)),
                                                fence(std::exchange(other.fence, VK_NULL_HANDLE)) {}

    VkFence &VkFence::operator=(VkFence &&other) noexcept {
        std::swap(device, other.device);
        std::swap(fence, other.fence);

        return *this;
    }

    VkFence::~VkFence() {
        if (device == nullptr)
            return;

        wait();
        vkDestroyFence(device->getDevice(), fence, nullptr);
    }

    void VkFence::wait(const uint64_t timeout) const {
        checkVulkanResult(
            vkWaitForFences(device->getDevice(), 1, &fence, VK_TRUE, timeout),
            "Fence was signaled with an error"
        );
    }

    void VkFence::reset() const {
        vkResetFences(device->getDevice(), 1, &fence);
    }

    ::VkFence VkFence::getFence() const { return fence; }
}
