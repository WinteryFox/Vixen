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

#ifdef OPENGL_ENABLED
#include "platform/opengl/OpenGLRenderingContext.h"
#include "platform/opengl/OpenGLRenderingDevice.h"
#endif

namespace Vixen {
    Window *DisplayServer::createWindow(
        const std::string &title,
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

        const auto &glfwWindow = glfwCreateWindow(resolution.x, resolution.y, title.c_str(), nullptr, nullptr);
        ASSERT_THROW(glfwWindow != nullptr, CantCreateError, "Failed to create window");

        auto *window = new Window{
            .window = glfwWindow,
            .surface = new Surface{
                .resolution = resolution,
                .hasFramebufferSizeChanged = false,
                .windowMode = mode,
                .vsyncMode = vsync
            }
        };

        glfwSetWindowUserPointer(window->window, window);
        glfwSetFramebufferSizeCallback(window->window, [](auto w, auto width, auto height) {
            const auto wind = static_cast<Window *>(glfwGetWindowUserPointer(w));
            wind->surface->resolution = {width, height};
            wind->surface->hasFramebufferSizeChanged = true;
        });

        setWindowedMode(window, mode);
        setVSyncMode(window, vsync);

        return window;
    }

    DisplayServer::DisplayServer(
        const std::string &applicationName,
        const glm::ivec3 &applicationVersion,
        const RenderingDriver driver,
        const WindowMode windowMode,
        const VSyncMode vsyncMode,
        const WindowFlags flags,
        const glm::ivec2 resolution
    ) : driver(driver),
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
                renderingContext = std::make_shared<VulkanRenderingContext>(applicationName, applicationVersion);
                break;
#endif

#ifdef D3D12_ENABLED
            case RenderingDriver::D3D12:
                renderingContext = std::make_shared<D3D12RenderingContext>();
                break;
#endif

#ifdef OPENGL_ENABLED
            case RenderingDriver::OpenGL:
                renderingContext = std::make_shared<OpenGLRenderingContext>();
                break;
#endif

            default:
                ASSERT_THROW(false, CantCreateError, "Unsupported rendering driver.");
        }

        mainWindow = createWindow(applicationName, windowMode, vsyncMode, flags, resolution);
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

#ifdef OPENGL_ENABLED
            case RenderingDriver::OpenGL:
                renderingDevice = std::make_shared<OpenGLRenderingDevice>();
                break;
#endif

            default:
                ASSERT_THROW(false, CantCreateError, "Unsupported rendering driver.");
        }
    }

    DisplayServer::~DisplayServer() {
        glfwSetWindowShouldClose(mainWindow->window, GLFW_TRUE);
        glfwDestroyWindow(mainWindow->window);
        glfwTerminate();

        delete mainWindow;
    }

    Window *DisplayServer::getMainWindow() const {
        return mainWindow;
    }

    bool DisplayServer::shouldClose(const Window *window) {
        return glfwWindowShouldClose(window->window) == GLFW_TRUE;
    }

    bool DisplayServer::update(Window *window) {
        glfwPollEvents();

        if (window->surface->hasFramebufferSizeChanged) {
            window->surface->hasFramebufferSizeChanged = false;

            int width;
            int height;
            glfwGetFramebufferSize(window->window, &width, &height);

            spdlog::trace("Framebuffer resized to {}x{}", width, height);

            if (width == 0 || height == 0) {
                while (width == 0 || height == 0) {
                    glfwGetFramebufferSize(window->window, &width, &height);
                    glfwWaitEvents();
                }

                return false;
            }

            return true;
        }

        return false;
    }

    void DisplayServer::setVisible(const Window *window, bool visible) {
        if (visible)
            glfwShowWindow(window->window);
        else
            glfwHideWindow(window->window);
    }

    void DisplayServer::center(const Window *window) {
        int w;
        int h;
        glfwGetWindowSize(window->window, &w, &h);

        const auto monitor = glfwGetWindowMonitor(window->window);
        const auto mode = glfwGetVideoMode(monitor);

        int x;
        int y;
        glfwGetMonitorPos(monitor, &x, &y);
        glfwSetWindowPos(
            window->window,
            x + mode->width / 2 - w / 2,
            y + mode->height / 2 - h / 2
        );
    }

    bool DisplayServer::getWindowMonitor(const Window *window, Monitor &m) {
        const auto monitor = glfwGetWindowMonitor(window->window);
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

    void DisplayServer::setWindowedMode(const Window *window, const WindowMode mode) {
        window->surface->windowMode = mode;

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

                glfwSetWindowAttrib(window->window, GLFW_DECORATED, false);
                glfwSetWindowAttrib(window->window, GLFW_FLOATING, true);
                break;

            case WindowMode::Windowed:
                glfwGetWindowPos(window->window, &x, &y);
                glfwGetWindowSize(window->window, &w, &h);
                glfwSetWindowAttrib(window->window, GLFW_DECORATED, true);
                glfwSetWindowAttrib(window->window, GLFW_FLOATING, false);
                break;

            case WindowMode::Minimized:
                glfwIconifyWindow(window->window);
                break;

            case WindowMode::Maximized:
                glfwMaximizeWindow(window->window);
                break;
        }

        glfwSetWindowMonitor(window->window, m, x, y, w, h, refreshRate);
    }

    void DisplayServer::setVSyncMode(Window *window, const VSyncMode mode) {
        window->surface->vsyncMode = mode;

        if (driver == RenderingDriver::OpenGL) {
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
    }

    void DisplayServer::getFramebufferSize(const Window *window, int &width, int &height) {
        glfwGetFramebufferSize(window->window, &width, &height);
    }

    std::shared_ptr<RenderingDevice> DisplayServer::getRenderingDevice() const {
        return renderingDevice;
    }

    std::shared_ptr<RenderingContext> DisplayServer::getRenderingContext() const {
        return renderingContext;
    }

    void DisplayServer::maximize(const Window *window) {
        glfwMaximizeWindow(window->window);
    }
}
