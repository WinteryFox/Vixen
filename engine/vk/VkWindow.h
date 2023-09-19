#pragma once

#include <volk.h>
#include "../Window.h"
#include "Vulkan.h"

namespace Vixen::Vk {
    class VkWindow : public Window {
        std::vector<VkImage> images;

    public:
        std::vector<const char *> requiredExtensions;

        VkWindow(const std::string &title, const uint32_t &width, const uint32_t &height, bool transparentFrameBuffer);

        VkSurfaceKHR createSurface(VkInstance instance) const;
    };
}
