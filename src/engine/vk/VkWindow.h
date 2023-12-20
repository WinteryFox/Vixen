#pragma once

#include "Vulkan.h"
#include "../Window.h"

namespace Vixen::Vk {
    class VkWindow : public Window {
        std::vector<VkImage> images;

        std::vector<const char*> requiredExtensions;

    public:
        VkWindow(
            const std::string& title,
            const uint32_t& width,
            const uint32_t& height,
            bool transparentFrameBuffer
        );

        VkSurfaceKHR createSurface(VkInstance instance) const;

        [[nodiscard]] std::vector<const char*> getRequiredExtensions() const;
    };
}
