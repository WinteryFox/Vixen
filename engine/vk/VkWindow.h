#pragma once

#include <Volk/volk.h>
#include "../Window.h"
#include "Macro.h"

namespace Vixen::Engine {
    class VkWindow : public Window {
    public:
        std::vector<const char *> requiredExtensions;

        VkWindow(const std::string &title, const uint32_t &width, const uint32_t &height, bool transparentFrameBuffer);

        VkSurfaceKHR createSurface(VkInstance instance) const;
    };
}
