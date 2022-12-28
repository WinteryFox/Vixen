#include "Window.h"

namespace Vixen::Engine {
    Window::Window(const std::string &title, const uint32_t &width, const uint32_t &height) : window(nullptr) {
        glfwSetErrorCallback([](int code, const char *message) {
            spdlog::error("[GLFW] {} ({})", message, code);
        });

        if (glfwInit() != GLFW_TRUE)
            throw std::runtime_error("Failed to create window");

        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        /*glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);*/
    }

    Window::~Window() {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    bool Window::shouldClose() const {
        return glfwWindowShouldClose(window) == GLFW_TRUE;
    }

    void Window::update() {
        glfwPollEvents();
    }

    void Window::setVisible(bool visible) {
        if (visible)
            glfwShowWindow(window);
        else
            glfwHideWindow(window);
    }

    void Window::center() {
        uint32_t width, height;
        glfwGetWindowSize(window, reinterpret_cast<int *>(&width), reinterpret_cast<int *>(&height));

        const auto primary = glfwGetPrimaryMonitor();
        const auto mode = glfwGetVideoMode(primary);
        uint32_t x, y;
        glfwGetMonitorPos(primary, reinterpret_cast<int *>(&x), reinterpret_cast<int *>(&y));
        glfwSetWindowPos(window, x + mode->width / 2 - width / 2, y + mode->height / 2 - height / 2);
    }
}
