#include "VulkanWindow.h"

#include "Vulkan.h"

namespace Vixen {
    VulkanWindow::VulkanWindow(
            const std::string &title,
            const uint32_t &width,
            const uint32_t &height,
            const bool transparentFrameBuffer
    ) : Window(title, width, height, transparentFrameBuffer) {
        spdlog::trace("Creating new Vulkan window");
        if (!glfwVulkanSupported()) {
            spdlog::error("Vulkan is not supported on this device");
            throw std::runtime_error("Vulkan is not supported on this device");
        }

        uint32_t count;
        // TODO: Do I need to delete this char**? No clue lol
        const char **extensions = glfwGetRequiredInstanceExtensions(&count);
        requiredExtensions.resize(count);
        for (uint32_t i = 0; i < count; i++)
            requiredExtensions[i] = extensions[i];

        glfwDefaultWindowHints();
    }

    VkSurfaceKHR VulkanWindow::createSurface(VkInstance instance) const {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        checkVulkanResult(
                glfwCreateWindowSurface(instance, getWindow(), nullptr, &surface),
                "Failed to create Vulkan BaseWindow surface"
        );
        return surface;
    }

    std::vector<const char*> VulkanWindow::getRequiredExtensions() const { return requiredExtensions; }
}
