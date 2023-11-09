#include "VkSemaphore.h"

namespace Vixen::Vk {
    VkSemaphore::VkSemaphore(const std::shared_ptr<Device> &device)
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

    VkSemaphore::VkSemaphore(VkSemaphore &&o) noexcept
            : device(o.device),
              semaphore(std::exchange(o.semaphore, VK_NULL_HANDLE)) {}

    VkSemaphore::~VkSemaphore() {
        vkDestroySemaphore(device->getDevice(), semaphore, nullptr);
    }

    ::VkSemaphore VkSemaphore::getSemaphore() const {
        return semaphore;
    }
}
