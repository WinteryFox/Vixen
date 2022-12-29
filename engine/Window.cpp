#include "Window.h"

namespace Vixen::Engine {
    Window::Window(const std::string &title, const uint32_t &width, const uint32_t &height, bool transparentFrameBuffer)
            : window(nullptr) {
        glfwSetErrorCallback([](int code, const char *message) {
            spdlog::error("[GLFW] {} ({})", message, code);
        });

        if (glfwInit() != GLFW_TRUE)
            throw std::runtime_error("Failed to create window");

        int count;
        auto primary = glfwGetPrimaryMonitor();
        auto glfwMonitors = glfwGetMonitors(&count);

        for (int i = 0; i < count; i++) {
            const auto &monitor = glfwMonitors[i];
            const auto &mode = glfwGetVideoMode(monitor);
            monitors[monitor] = Monitor{
                    std::string(glfwGetMonitorName(monitor)),
                    mode->width,
                    mode->height,
                    mode->refreshRate,
                    mode->blueBits,
                    mode->redBits,
                    mode->greenBits,
                    primary == monitor
            };
        }

        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, transparentFrameBuffer ? GLFW_TRUE : GLFW_FALSE);
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

        const auto monitor = glfwGetPrimaryMonitor();
        const auto mode = glfwGetVideoMode(monitor);
        uint32_t x, y;
        glfwGetMonitorPos(monitor, reinterpret_cast<int *>(&x), reinterpret_cast<int *>(&y));
        glfwSetWindowPos(window, x + mode->width / 2 - width / 2, y + mode->height / 2 - height / 2);
    }

    std::unique_ptr<Monitor> Window::getMonitor() {
        const auto &monitor = glfwGetWindowMonitor(window);
        if (monitor == nullptr)
            return nullptr;

        const auto &primary = glfwGetPrimaryMonitor();
        const auto &mode = glfwGetVideoMode(monitor);

        return std::make_unique<Monitor>(Monitor{
                std::string(glfwGetMonitorName(monitor)),
                mode->width,
                mode->height,
                mode->refreshRate,
                mode->blueBits,
                mode->redBits,
                mode->greenBits,
                primary == monitor
        });
    }

    std::unordered_map<GLFWmonitor*, Monitor> Window::getMonitors() {
        return monitors;
    }

    void Window::setWindowedMode(Mode mode, int width, int height, int x, int y, int refreshRate) {
        glfwSetWindowMonitor(window, nullptr, x, y, width, height, refreshRate);
    }

    void Window::setClearColor(float r, float g, float b, float a) {
        clearColor = glm::vec4(r, g, b, a);
    }
}
