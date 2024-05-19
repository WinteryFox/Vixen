#include "VulkanFence.h"

#include "device/VulkanDevice.h"

namespace Vixen {
    VulkanFence::VulkanFence(const std::shared_ptr<VulkanDevice> &device, const bool createSignaled)
        : device(device),
          fence(VK_NULL_HANDLE) {
        const VkFenceCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = static_cast<VkFenceCreateFlags>(createSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0)
        };

        vkCreateFence(device->getDevice(), &info, nullptr, &fence);
    }

    VulkanFence::VulkanFence(VulkanFence &&other) noexcept: device(std::exchange(other.device, VK_NULL_HANDLE)),
                                                fence(std::exchange(other.fence, VK_NULL_HANDLE)) {}

    VulkanFence &VulkanFence::operator=(VulkanFence &&other) noexcept {
        std::swap(device, other.device);
        std::swap(fence, other.fence);

        return *this;
    }

    VulkanFence::~VulkanFence() {
        if (device == nullptr)
            return;

        wait();
        vkDestroyFence(device->getDevice(), fence, nullptr);
    }

    void VulkanFence::wait(const uint64_t timeout) const {
        checkVulkanResult(
            vkWaitForFences(device->getDevice(), 1, &fence, VK_TRUE, timeout),
            "Fence was signaled with an error"
        );
    }

    void VulkanFence::reset() const {
        vkResetFences(device->getDevice(), 1, &fence);
    }

    ::VkFence VulkanFence::getFence() const { return fence; }
}
