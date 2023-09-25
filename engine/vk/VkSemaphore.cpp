#include "VkSemaphore.h"

namespace Vixen::Vk {
    VkSemaphore::VkSemaphore(const std::shared_ptr<Device> &device)
            : semaphore(VK_NULL_HANDLE) {
        VkSemaphoreCreateInfo info{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        vkCreateSemaphore(device->getDevice(), &info, nullptr, &semaphore);
    }

    VkSemaphore::~VkSemaphore() {
        vkDestroySemaphore(device->getDevice(), semaphore, nullptr);
    }
}
