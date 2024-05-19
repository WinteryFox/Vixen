#include "VulkanSemaphore.h"

namespace Vixen {
    VulkanSemaphore::VulkanSemaphore(const std::shared_ptr<VulkanDevice> &device)
            : device(device),
              semaphore(VK_NULL_HANDLE) {
        VkSemaphoreCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        checkVulkanResult(
                vkCreateSemaphore(device->getDevice(), &info, nullptr, &semaphore),
                "Failed to create semaphore"
        );
    }

    VulkanSemaphore::VulkanSemaphore(VulkanSemaphore &&o) noexcept
            : device(o.device),
              semaphore(std::exchange(o.semaphore, VK_NULL_HANDLE)) {}

    VulkanSemaphore::~VulkanSemaphore() {
        vkDestroySemaphore(device->getDevice(), semaphore, nullptr);
    }

    ::VkSemaphore VulkanSemaphore::getSemaphore() const {
        return semaphore;
    }
}
