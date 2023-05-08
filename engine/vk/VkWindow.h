#pragma once

#include <volk.h>
#include "../Window.h"
#include "Vulkan.h"

namespace Vixen::Engine {
    class VkWindow : public Window {
        VkSwapchainKHR swapchain;

        std::vector<VkImage> images;

    public:
        std::vector<const char *> requiredExtensions;

        VkWindow(const std::string &title, const uint32_t &width, const uint32_t &height, bool transparentFrameBuffer);

        VkSurfaceKHR createSurface(VkInstance instance) const;
    };
}
