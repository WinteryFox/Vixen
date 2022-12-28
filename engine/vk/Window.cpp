#include "Window.h"

namespace Vixen::Engine::Vk {
    Window::Window(const std::string &title, const uint32_t &width, const uint32_t &height) :
            Vixen::Engine::Window(title, width, height) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!window)
            glfwTerminate();
        glfwDefaultWindowHints();
    }
}
