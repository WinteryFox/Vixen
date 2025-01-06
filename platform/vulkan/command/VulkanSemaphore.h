#pragma once

#include "core/command/Semaphore.h"

namespace Vixen {
    struct VulkanSemaphore : Semaphore {
        VkSemaphore semaphore;
    };
}
