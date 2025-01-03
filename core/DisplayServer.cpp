#include "DisplayServer.h"

#include "error/CantCreateError.h"
#include "error/Macros.h"

#ifdef VULKAN_ENABLED
#include "platform/vulkan/VulkanRenderingContext.h"
#include "platform/vulkan/VulkanRenderingDevice.h"
#endif

#ifdef D3D12_ENABLED
#include "platform/d3d12/D3D12RenderingContext.h"
#include "platform/d3d12/D3D12RenderingDevice.h"
#endif

namespace Vixen {
    void DisplayServer::createWindow(
        const WindowMode mode,
        const VSyncMode vsync,
        const WindowFlags flags,
        const glm::ivec2 resolution
    ) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, flags & WindowFlags::Resizable ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, flags & WindowFlags::Transparent ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, flags & WindowFlags::Borderless ? GLFW_FALSE : GLFW_TRUE);
        glfwWindowHint(GLFW_FLOATING, flags & WindowFlags::AlwaysOnTop ? GLFW_TRUE : GLFW_FALSE);

        this->mainWindow = glfwCreateWindow(resolution.x, resolution.y, "", nullptr, nullptr);
        ASSERT_THROW(mainWindow != nullptr, CantCreateError, "Failed to create window");

        glfwSetWindowUserPointer(mainWindow, this);
        glfwSetFramebufferSizeCallback(mainWindow, [](auto w, auto, auto) {
            const auto wind = static_cast<DisplayServer *>(glfwGetWindowUserPointer(w));
            wind->framebufferSizeChanged = true;
        });

        setWindowedMode(mode);
        setVSyncMode(vsync);
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
        ASSERT_THROW(glfwInit() != GLFW_FALSE, CantCreateError,
                     "Failed to initialize GLFW.\n"
                     "glfwInit failed.")

        glfwSetErrorCallback([](int code, const char *message) {
            spdlog::error("[GLFW] {} ({})", message, code);
        });

        switch (driver) {
#ifdef VULKAN_ENABLED
            case RenderingDriver::Vulkan:
                renderingContext = std::make_shared<VulkanRenderingContext>();
                break;
#endif

#ifdef D3D12_ENABLED
            case RenderingDriver::D3D12:
                renderingContext = std::make_shared<D3D12RenderingContext>();
                break;
#endif

            default:
                ASSERT_THROW(false, CantCreateError, "Unsupported rendering driver.");
        }

        createWindow(mode, vsync, flags, resolution);
        ASSERT_THROW(mainWindow, CantCreateError, "Failed to create window.");

        switch (driver) {
#ifdef VULKAN_ENABLED
            case RenderingDriver::Vulkan:
                // TODO: Hardcoded physical device index
                renderingDevice = std::make_shared<VulkanRenderingDevice>(
                    std::dynamic_pointer_cast<VulkanRenderingContext>(renderingContext), 0);
                break;
#endif

#ifdef D3D12_ENABLED
            case RenderingDriver::D3D12:
                renderingDevice = std::make_shared<D3D12RenderingDevice>(std::dynamic_pointer_cast<D3D12RenderingContext>(renderingContext));
                break;
#endif

            default:
                ASSERT_THROW(false, CantCreateError, "Unsupported rendering driver.");
        }
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

    void DisplayServer::setVSyncMode(const VSyncMode mode) const {
        switch (mode) {
            case VSyncMode::Disabled:
                glfwSwapInterval(0);
                break;

            case VSyncMode::Enabled:
            case VSyncMode::Adaptive:
            case VSyncMode::Mailbox:
                glfwSwapInterval(1);
                break;
        }
    }

    void DisplayServer::getFramebufferSize(int &width, int &height) const {
        glfwGetFramebufferSize(mainWindow, &width, &height);
    }

    std::shared_ptr<RenderingDevice> DisplayServer::getRenderingDevice() const {
        return renderingDevice;
    }

    std::shared_ptr<RenderingContext> DisplayServer::getRenderingContext() const {
        return renderingContext;
    }

    void DisplayServer::maximize() const {
        glfwMaximizeWindow(mainWindow);
    }
}
