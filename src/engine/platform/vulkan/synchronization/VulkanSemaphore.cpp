#include "VulkanSemaphore.h"

#include "device/VulkanDevice.h"

namespace Vixen {
    VulkanSemaphore::VulkanSemaphore(const std::shared_ptr<VulkanDevice> &device)
        : device(device),
          semaphore(VK_NULL_HANDLE) {
        VkSemaphoreCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
        };

        checkVulkanResult(
            vkCreateSemaphore(device->getDevice(), &info, nullptr, &semaphore),
            "Failed to create semaphore"
        );
    }

    VulkanSemaphore::VulkanSemaphore(VulkanSemaphore &&o) noexcept
        : device(std::exchange(o.device, nullptr)),
          semaphore(std::exchange(o.semaphore, VK_NULL_HANDLE)) {}

    VulkanSemaphore &VulkanSemaphore::operator=(VulkanSemaphore &&o) noexcept {
        std::swap(device, o.device);
        std::swap(semaphore, o.semaphore);

        return *this;
    }

    VulkanSemaphore::~VulkanSemaphore() {
        if (device)
            vkDestroySemaphore(device->getDevice(), semaphore, nullptr);
    }

    ::VkSemaphore VulkanSemaphore::getSemaphore() const {
        return semaphore;
    }
}
