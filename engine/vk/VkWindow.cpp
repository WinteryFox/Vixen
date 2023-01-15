#include "VkWindow.h"

namespace Vixen::Engine::Vk {
    VkWindow::VkWindow(const std::string &title, const uint32_t &width, const uint32_t &height,
                       bool transparentFrameBuffer)
            : Vixen::Engine::Window(transparentFrameBuffer) {
        spdlog::trace("Creating new Vulkan window");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#ifdef DEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

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

#ifdef __APPLE__
        requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

        glfwDefaultWindowHints();
    }

    VkSurfaceKHR VkWindow::createSurface(VkInstance instance) {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface), "Failed to create Vulkan Window surface")
        return surface;
    }
}
