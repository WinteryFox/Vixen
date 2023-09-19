#include "VkWindow.h"

namespace Vixen::Vk {
    VkWindow::VkWindow(
            const std::string &title,
            const uint32_t &width,
            const uint32_t &height,
            bool transparentFrameBuffer
    ) : Vixen::Window(transparentFrameBuffer) {
        spdlog::trace("Creating new Vulkan window");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr);
        if (!window) {
            spdlog::error("Failed to create window");
            glfwDestroyWindow(window);
            glfwTerminate();
            throw std::runtime_error("Failed to create window");
        }

        if (!glfwVulkanSupported()) {
            glfwDestroyWindow(window);
            glfwTerminate();
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

    VkSurfaceKHR VkWindow::createSurface(VkInstance instance) const {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        checkVulkanResult(
                glfwCreateWindowSurface(instance, window, nullptr, &surface),
                "Failed to create Vulkan BaseWindow surface"
        );
        return surface;
    }
}
