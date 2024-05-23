#include "Window.h"

namespace Vixen {
    Window::Window(
        const std::string &title,
        const uint32_t &width,
        const uint32_t &height,
        const bool transparentFrameBuffer
    ) : window(nullptr) {
        glfwSetErrorCallback([](int code, const char *message) {
            spdlog::error("[GLFW] {} ({})", message, code);
        });

        if (glfwInit() != GLFW_TRUE)
            throw std::runtime_error("Failed to create window");

        int count;
        const auto primary = glfwGetPrimaryMonitor();
        const auto glfwMonitors = glfwGetMonitors(&count);

        monitor = glfwGetPrimaryMonitor(); // TODO
        for (int i = 0; i < count; i++) {
            const auto &m = glfwMonitors[i];
            const auto &mode = glfwGetVideoMode(m);
            monitors[m] = Monitor{
                std::string(glfwGetMonitorName(m)),
                mode->width,
                mode->height,
                mode->refreshRate,
                mode->blueBits,
                mode->redBits,
                mode->greenBits,
                primary == m
            };
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, transparentFrameBuffer ? GLFW_TRUE : GLFW_FALSE);
        /*glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);*/

        window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr);
        if (!window) {
            spdlog::error("Failed to create window");
            glfwDestroyWindow(window);
            glfwTerminate();
            throw std::runtime_error("Failed to create window");
        }

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, [](auto w, auto, auto) {
            const auto wind = static_cast<Window *>(glfwGetWindowUserPointer(w));
            wind->framebufferSizeChanged = true;
        });
    }

    Window::Window(Window &&other) noexcept
        : monitor(std::exchange(other.monitor, nullptr)),
          window(std::exchange(other.window, nullptr)),
          monitors(std::move(other.monitors)) {}

    Window &Window::operator=(Window &&other) noexcept {
        std::swap(window, other.window);
        std::swap(monitor, other.monitor);
        std::swap(monitors, other.monitors);

        return *this;
    }

    Window::~Window() {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    GLFWwindow *Window::getWindow() const { return window; }

    bool Window::isFramebufferSizeChanged() const { return framebufferSizeChanged; }


    bool Window::shouldClose() const {
        return glfwWindowShouldClose(window) == GLFW_TRUE;
    }

    bool Window::update() {
        glfwPollEvents();

        if (framebufferSizeChanged) {
            framebufferSizeChanged = false;

            int width;
            int height;
            glfwGetFramebufferSize(window, &width, &height);

            spdlog::trace("Framebuffer resized to {}x{}", width, height);

            if (width == 0 || height == 0) {
                while (width == 0 || height == 0) {
                    glfwGetFramebufferSize(window, &width, &height);
                    glfwWaitEvents();
                }

                return false;
            }

            return true;
        }

        return false;
    }

    void Window::setVisible(bool visible) const {
        if (visible)
            glfwShowWindow(window);
        else
            glfwHideWindow(window);
    }

    void Window::center() const {
        int w;
        int h;
        glfwGetWindowSize(window, &w, &h);

        const auto mode = glfwGetVideoMode(monitor);

        int x;
        int y;
        glfwGetMonitorPos(monitor, &x, &y);
        glfwSetWindowPos(
            window,
            x + mode->width / 2 - w / 2,
            y + mode->height / 2 - h / 2
        );
    }

    std::unique_ptr<Monitor> Window::getMonitor() const {
        if (monitor == nullptr)
            return nullptr;

        const auto &primary = glfwGetPrimaryMonitor();
        const auto &mode = glfwGetVideoMode(monitor);

        return std::make_unique<Monitor>(
            std::string(glfwGetMonitorName(monitor)),
            mode->width,
            mode->height,
            mode->refreshRate,
            mode->blueBits,
            mode->redBits,
            mode->greenBits,
            primary == monitor
        );
    }

    std::unordered_map<GLFWmonitor *, Monitor> Window::getMonitors() const {
        return monitors;
    }

    void Window::setWindowedMode(const Mode mode) const {
        int w;
        int h;
        int x;
        int y;
        int refreshRate = 0;
        GLFWmonitor *m = nullptr;

        const auto &videoMode = glfwGetVideoMode(monitor);

        switch (mode) {
            case Mode::Fullscreen:
            case Mode::BorderlessFullscreen:
                m = monitor;
                refreshRate = videoMode->refreshRate;
                w = videoMode->width;
                h = videoMode->height;

                glfwSetWindowAttrib(window, GLFW_DECORATED, false);
                glfwSetWindowAttrib(window, GLFW_FLOATING, true);
                break;

            case Mode::Windowed:
                glfwGetWindowPos(window, &x, &y);
                glfwGetWindowSize(window, &w, &h);
                glfwSetWindowAttrib(window, GLFW_DECORATED, true);
                glfwSetWindowAttrib(window, GLFW_FLOATING, false);
                break;
        }

        glfwSetWindowMonitor(window, m, x, y, w, h, refreshRate);
    }

    void Window::getFramebufferSize(int &width, int &height) const {
        glfwGetFramebufferSize(window, &width, &height);
    }

    void Window::maximize() const {
        glfwMaximizeWindow(window);
    }
}
