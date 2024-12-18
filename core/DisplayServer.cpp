#include "DisplayServer.h"

#include "platform/vulkan/VulkanRenderingDevice.h"
#include "error/CantCreateError.h"
#include "error/Macros.h"
#include "platform/vulkan/VulkanRenderingContext.h"

namespace Vixen {
    GLFWwindow *DisplayServer::createWindow(
        const WindowMode mode,
        const VSyncMode vsync,
        const WindowFlags flags,
        const glm::ivec2 resolution
    ) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, (flags & Resizable) == 0 ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, (flags & Transparent) == 0 ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, (flags & Borderless) == 0 ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_FLOATING, (flags & AlwaysOnTop) == 0 ? GLFW_TRUE : GLFW_FALSE);

        const auto window = glfwCreateWindow(resolution.x, resolution.y, "", nullptr, nullptr);
        if (!window) {
            spdlog::error("Failed to create window");
            glfwDestroyWindow(window);
            glfwTerminate();
            throw std::runtime_error("Failed to create window");
        }

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, [](auto w, auto, auto) {
            const auto wind = static_cast<DisplayServer *>(glfwGetWindowUserPointer(w));
            wind->framebufferSizeChanged = true;
        });

        return window;
    }

    DisplayServer::DisplayServer(
        const RenderingDriver driver,
        const WindowMode mode,
        const VSyncMode vsync,
        const WindowFlags flags,
        const glm::ivec2 resolution
    ) : driver(driver),
        resolution(resolution),
        mainWindow(nullptr) {
#ifdef VULKAN_ENABLED
        if (driver == RenderingDriver::Vulkan)
            renderingContext = std::make_shared<VulkanRenderingContext>();
#endif

        if (!renderingContext)
            throw std::runtime_error("Failed to create rendering context");

        mainWindow = createWindow(mode, vsync, flags, resolution);
        ASSERT_THROW(mainWindow, CantCreateError, "Failed to create window");

#ifdef VULKAN_ENABLED
        if (driver == RenderingDriver::Vulkan)
            renderingDevice = std::make_shared<VulkanRenderingDevice>();
#endif
        ASSERT_THROW(renderingDevice, CantCreateError, "Failed to create rendering device");
    }

    DisplayServer::DisplayServer(DisplayServer &&other) noexcept
        : driver(other.driver),
          resolution(other.resolution),
          mainWindow(std::exchange(other.mainWindow, nullptr)) {
    }

    DisplayServer &DisplayServer::operator=(DisplayServer &&other) noexcept {
        driver = other.driver;
        resolution = other.resolution;

        std::swap(mainWindow, other.mainWindow);

        return *this;
    }

    DisplayServer::~DisplayServer() {
        glfwSetWindowShouldClose(mainWindow, GLFW_TRUE);
        glfwDestroyWindow(mainWindow);
        glfwTerminate();
    }

    GLFWwindow *DisplayServer::getWindow() const { return mainWindow; }

    bool DisplayServer::isFramebufferSizeChanged() const { return framebufferSizeChanged; }


    bool DisplayServer::shouldClose() const {
        return glfwWindowShouldClose(mainWindow) == GLFW_TRUE;
    }

    bool DisplayServer::update() {
        glfwPollEvents();

        if (framebufferSizeChanged) {
            framebufferSizeChanged = false;

            int width;
            int height;
            glfwGetFramebufferSize(mainWindow, &width, &height);

            spdlog::trace("Framebuffer resized to {}x{}", width, height);

            if (width == 0 || height == 0) {
                while (width == 0 || height == 0) {
                    glfwGetFramebufferSize(mainWindow, &width, &height);
                    glfwWaitEvents();
                }

                return false;
            }

            return true;
        }

        return false;
    }

    void DisplayServer::setVisible(bool visible) const {
        if (visible)
            glfwShowWindow(mainWindow);
        else
            glfwHideWindow(mainWindow);
    }

    void DisplayServer::center() const {
        int w;
        int h;
        glfwGetWindowSize(mainWindow, &w, &h);

        const auto monitor = glfwGetWindowMonitor(mainWindow);
        const auto mode = glfwGetVideoMode(monitor);

        int x;
        int y;
        glfwGetMonitorPos(monitor, &x, &y);
        glfwSetWindowPos(
            mainWindow,
            x + mode->width / 2 - w / 2,
            y + mode->height / 2 - h / 2
        );
    }

    bool DisplayServer::getWindowMonitor(Monitor &m) const {
        const auto monitor = glfwGetWindowMonitor(mainWindow);
        if (monitor == nullptr)
            return false;

        const auto &primary = glfwGetPrimaryMonitor();
        const auto &mode = glfwGetVideoMode(monitor);

        m = Monitor(
            std::string(glfwGetMonitorName(monitor)),
            mode->width,
            mode->height,
            mode->refreshRate,
            mode->blueBits,
            mode->redBits,
            mode->greenBits,
            primary == monitor
        );

        return true;
    }

    std::vector<Monitor> DisplayServer::getMonitors() {
        int count;
        const auto glfwMonitors = glfwGetMonitors(&count);
        const auto primaryMonitor = glfwGetPrimaryMonitor();

        std::vector<Monitor> monitors;
        monitors.reserve(count);
        for (int i = 0; i < count; i++) {
            const auto &m = glfwMonitors[i];
            const auto &mode = glfwGetVideoMode(m);
            monitors.emplace_back(
                std::string(glfwGetMonitorName(m)),
                mode->width,
                mode->height,
                mode->refreshRate,
                mode->blueBits,
                mode->redBits,
                mode->greenBits,
                m == primaryMonitor
            );
        }

        return monitors;
    }

    void DisplayServer::setWindowedMode(const WindowMode mode) const {
        int w;
        int h;
        int x;
        int y;
        int refreshRate = 0;
        GLFWmonitor *m = nullptr;

        const auto monitor = glfwGetPrimaryMonitor();
        const auto &videoMode = glfwGetVideoMode(monitor);

        switch (mode) {
            case WindowMode::ExclusiveFullscreen:
            case WindowMode::BorderlessFullscreen:
                m = monitor;
                refreshRate = videoMode->refreshRate;
                w = videoMode->width;
                h = videoMode->height;

                glfwSetWindowAttrib(mainWindow, GLFW_DECORATED, false);
                glfwSetWindowAttrib(mainWindow, GLFW_FLOATING, true);
                break;

            case WindowMode::Windowed:
                glfwGetWindowPos(mainWindow, &x, &y);
                glfwGetWindowSize(mainWindow, &w, &h);
                glfwSetWindowAttrib(mainWindow, GLFW_DECORATED, true);
                glfwSetWindowAttrib(mainWindow, GLFW_FLOATING, false);
                break;

            case WindowMode::Minimized:
                glfwIconifyWindow(mainWindow);
                break;

            case WindowMode::Maximized:
                glfwMaximizeWindow(mainWindow);
                break;
        }

        glfwSetWindowMonitor(mainWindow, m, x, y, w, h, refreshRate);
    }

    void DisplayServer::getFramebufferSize(int &width, int &height) const {
        glfwGetFramebufferSize(mainWindow, &width, &height);
    }

    void DisplayServer::maximize() const {
        glfwMaximizeWindow(mainWindow);
    }
}
