#include "Window.h"

namespace Vixen::Engine::Vk {
    Window::Window(const std::string &title, const uint32_t &width, const uint32_t &height, bool transparentFrameBuffer)
            : Vixen::Engine::Window(transparentFrameBuffer) {
        spdlog::trace("Creating new Vulkan window");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#ifdef DEBUG

            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

        window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr);
        if (!window) {
            spdlog::error("Failed to create window");
            glfwTerminate();
            throw std::runtime_error("Failed to create window");
        }

        glfwDefaultWindowHints();
    }
}
