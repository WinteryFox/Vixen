#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "DisplayServer.h"

#include "RenderingDevice.h"
#include "error/CantCreateError.h"
#include "error/Macros.h"
#include "platform/vulkan/VulkanSurface.h"

#ifdef VULKAN_ENABLED
#include "platform/vulkan/VulkanRenderingContextDriver.h"
#endif

#ifdef D3D12_ENABLED
#include "platform/d3d12/D3D12RenderingContext.h"
#endif

#ifdef OPENGL_ENABLED
#include "platform/opengl/OpenGLRenderingContext.h"
#endif

namespace Vixen {
    Window *DisplayServer::createWindow(
        const std::string &title,
        const WindowMode mode,
        const VSyncMode vsync,
        const WindowFlags flags,
        const glm::uvec2 resolution
    ) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, flags & WindowFlags::Resizable ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, flags & WindowFlags::Transparent ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, flags & WindowFlags::Borderless ? GLFW_FALSE : GLFW_TRUE);
        glfwWindowHint(GLFW_FLOATING, flags & WindowFlags::AlwaysOnTop ? GLFW_TRUE : GLFW_FALSE);

        auto *handle = glfwCreateWindow(static_cast<int>(resolution.x), static_cast<int>(resolution.y),
                                        title.c_str(), nullptr, nullptr);
        if (!handle)
            error<CantCreateError>("Failed to create window");

        Surface *surface = nullptr;
        switch (driver) {
#ifdef VULKAN_ENABLED
            case RenderingDriver::Vulkan:
                surface = new VulkanSurface();
                break;
#endif

#ifdef D3D12_ENABLED
            case RenderingDriver::D3D12:
                surface = new D3D12Surface();
            break;
#endif

#ifdef OPENGL_ENABLED
            case RenderingDriver::OpenGL:
                surface = new OpenGLSurface();
            break;
#endif

            default:
                break;
        }

        if (!surface)
            error<CantCreateError>("Failed to detect surface type for current driver.");

        surface->resolution = resolution;
        surface->hasFramebufferSizeChanged = false;
        surface->windowMode = mode;
        surface->vsyncMode = vsync;

        auto *window = new Window{
            .window = handle,
            .surface = surface
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
        const glm::uvec2 resolution
    ) : driver(driver) {
        if (glfwInit() != GLFW_TRUE)
            error<CantCreateError>("Failed to initialize GLFW.\n"
                "glfwInit failed.");

        glfwSetErrorCallback([](int code, const char *message) {
            spdlog::error("[GLFW] {} ({})", message, code);
        });

        switch (driver) {
#ifdef VULKAN_ENABLED
            case RenderingDriver::Vulkan:
                renderingContextDriver = new VulkanRenderingContextDriver(applicationName, applicationVersion);
                break;
#endif

#ifdef D3D12_ENABLED
            case RenderingDriver::D3D12:
                renderingContextDriver = new D3D12Rendering(applicationName, applicationVersion);
                break;
#endif

#ifdef OPENGL_ENABLED
            case RenderingDriver::OpenGL:
                renderingContextDriver = new VulkanRenderingContextDriver(applicationName, applicationVersion);
                break;
            }
#endif

            default:
                throw std::runtime_error("Failed to create display server");
        }

        mainWindow = createWindow(applicationName, windowMode, vsyncMode, flags, resolution);
        mainWindow->surface = renderingContextDriver->createSurface(mainWindow);

        renderingDevice = new RenderingDevice(renderingContextDriver, mainWindow);
        const auto swapchain = renderingDevice->createScreen(mainWindow);
        if (!swapchain)
            throw CantCreateError("Failed to create rendering device screen");
        mainWindow->swapchain = swapchain.value();
    }

    DisplayServer::~DisplayServer() {
        renderingDevice->destroyScreen(mainWindow);

        delete renderingDevice;
        delete renderingContextDriver;

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

    void DisplayServer::update(Window *window) {
        glfwPollEvents();

        if (!renderingDevice->prepareScreenForDrawing(window))
            throw CantCreateError("Failed to prepare screen for drawing.");

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

                return;
            }
        }

        renderingDevice->swapBuffers(true);
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

        m = {
            std::string(glfwGetMonitorName(monitor)),
            mode->width,
            mode->height,
            mode->refreshRate,
            mode->blueBits,
            mode->redBits,
            mode->greenBits,
            primary == monitor
        };

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

        int width;
        int height;
        int x;
        int y;

        int refreshRate = 0;
        GLFWmonitor *monitor = nullptr;
        const GLFWvidmode *videoMode = nullptr;

        switch (mode) {
                using enum WindowMode;

            case ExclusiveFullscreen:
            case BorderlessFullscreen:
                monitor = glfwGetPrimaryMonitor();
                videoMode = glfwGetVideoMode(monitor);
                refreshRate = videoMode->refreshRate;
                width = videoMode->width;
                height = videoMode->height;

                glfwSetWindowAttrib(window->window, GLFW_DECORATED, GLFW_FALSE);
                glfwSetWindowAttrib(window->window, GLFW_FLOATING, GLFW_TRUE);
                break;

            case Windowed:
                glfwGetWindowPos(window->window, &x, &y);
                glfwGetWindowSize(window->window, &width, &height);
                glfwSetWindowAttrib(window->window, GLFW_DECORATED, GLFW_TRUE);
                glfwSetWindowAttrib(window->window, GLFW_FLOATING, GLFW_FALSE);
                break;

            case Minimized:
                glfwIconifyWindow(window->window);
                break;

            case Maximized:
                glfwMaximizeWindow(window->window);
                break;
        }

        glfwSetWindowMonitor(window->window, monitor, x, y, width, height, refreshRate);
    }

    void DisplayServer::setVSyncMode(const Window *window, const VSyncMode mode) {
        window->surface->vsyncMode = mode;

        if (driver == RenderingDriver::OpenGL) {
            switch (mode) {
                    using enum VSyncMode;

                case Disabled:
                    glfwSwapInterval(0);
                    break;

                case Enabled:
                case Adaptive:
                case Mailbox:
                    glfwSwapInterval(1);
                    break;
            }
        }
    }

    void DisplayServer::getFramebufferSize(const Window *window, int &width, int &height) {
        glfwGetFramebufferSize(window->window, &width, &height);
    }

    void DisplayServer::maximize(const Window *window) {
        glfwMaximizeWindow(window->window);
    }
}
