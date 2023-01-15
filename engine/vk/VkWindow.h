#pragma once

#include <vulkan/vulkan.hpp>
#include "../Window.h"

namespace Vixen::Engine::Vk {
    class VkWindow : public Vixen::Engine::Window {
        friend class Surface;

    public:
        std::vector<const char *> requiredExtensions;

        VkWindow(const std::string &title, const uint32_t &width, const uint32_t &height, bool transparentFrameBuffer);

        VkSurfaceKHR createSurface(VkInstance instance) const;
    };
}
