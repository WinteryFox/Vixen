#pragma once

#include <volk.h>

#include "core/rendering/Surface.h"

namespace Vixen {
    struct VulkanSurface : Surface {
        VkSurfaceKHR surface;
    };
}
