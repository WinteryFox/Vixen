#pragma once

#include <volk.h>

#include "core/Surface.h"

namespace Vixen {
    struct VulkanSurface : Surface {
        VkSurfaceKHR surface;
    };
}
