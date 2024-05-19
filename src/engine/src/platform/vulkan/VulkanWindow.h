#pragma once

#include <Volk/volk.h>
#include <core/Window.h>

namespace Vixen {
    class VulkanWindow : public Window {
        std::vector<const char*> requiredExtensions;

    public:
        VulkanWindow(
            const std::string& title,
            const uint32_t& width,
            const uint32_t& height,
            bool transparentFrameBuffer
        );

        VkSurfaceKHR createSurface(VkInstance instance) const;

        [[nodiscard]] std::vector<const char*> getRequiredExtensions() const;
    };
}
