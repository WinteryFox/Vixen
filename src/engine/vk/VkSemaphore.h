#pragma once

#include "Device.h"

namespace Vixen::Vk {
    class VkSemaphore {
        std::shared_ptr<Device> device;

        ::VkSemaphore semaphore;

    public:
        explicit VkSemaphore(const std::shared_ptr<Device> &device);

        VkSemaphore(const VkSemaphore &) = delete;

        VkSemaphore(VkSemaphore &&o) noexcept;

        VkSemaphore &operator=(const VkSemaphore &) = delete;

        ~VkSemaphore();

        [[nodiscard]] ::VkSemaphore getSemaphore() const;
    };
}
