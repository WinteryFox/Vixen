#pragma once

#include "../VulkanDevice.h"

namespace Vixen {
    class VulkanSemaphore {
        std::shared_ptr<VulkanDevice> device;

        ::VkSemaphore semaphore;

    public:
        explicit VulkanSemaphore(const std::shared_ptr<VulkanDevice> &device);

        VulkanSemaphore(const VulkanSemaphore &) = delete;

        VulkanSemaphore(VulkanSemaphore &&o) noexcept;

        VulkanSemaphore &operator=(const VulkanSemaphore &) = delete;

        ~VulkanSemaphore();

        [[nodiscard]] ::VkSemaphore getSemaphore() const;
    };
}
