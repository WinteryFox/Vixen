#pragma once

#include <expected>
#include <map>
#include <string>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Monitor.h"
#include "core/rendering/RenderingDriver.h"
#include "VSyncMode.h"
#include "WindowFlags.h"
#include "WindowMode.h"
#include "core/error/Error.h"

struct GLFWwindow;

namespace Vixen {
    class RenderingContextDriver;
    class RenderingDevice;
    struct Window;

    class DisplayServer final {
        RenderingDriver driver;

        RenderingContextDriver* renderingContextDriver;
        RenderingDevice* renderingDevice;

        std::map<GLFWwindow*, Window*> windows;

        Window* mainWindow = nullptr;

        auto createWindow(
            const std::string& title,
            WindowMode mode,
            VSyncMode vsync,
            WindowFlags flags,
            glm::uvec2 resolution
        ) -> std::expected<Window*, Error>;

        Window* getWindowFromHandle(GLFWwindow* handle);

    public:
        DisplayServer(
            const std::string& applicationName,
            const glm::ivec3& applicationVersion,
            RenderingDriver driver,
            WindowMode windowMode,
            VSyncMode vsyncMode,
            WindowFlags flags,
            glm::uvec2 resolution
        );

        DisplayServer(const DisplayServer&) = delete;

        DisplayServer& operator=(const DisplayServer&) = delete;

        DisplayServer(DisplayServer&& other) noexcept = delete;

        DisplayServer& operator=(DisplayServer&& other) noexcept = delete;

        ~DisplayServer();

        [[nodiscard]] Window* getMainWindow() const;

        [[nodiscard]] bool isFramebufferSizeChanged() const;

        /**
         * Has the current window been requested to close?
         * @return True if the window should close, false if not.
         */
        bool shouldClose(const Window* window);

        /**
         * Polls window events and processes them.
         * @return Returns true if the framebuffer size has been resized, false if not.
         */
        void update(Window* window);

        /**
         * Sets the visibility of the window.
         * @param visible True for visible, false for hidden.
         */
        void setVisible(const Window* window, bool visible);

        /**
         * Centers the window on the current monitor.
         */
        void center(const Window* window);

        void maximize(const Window* window);

        void setWindowedMode(Window *window, WindowMode mode);

        void setVSyncMode(Window* window, VSyncMode mode) const;

        [[nodiscard]] bool getWindowMonitor(const Window* window, Monitor& m) const;

        std::vector<Monitor> getMonitors();

        void getFramebufferSize(const Window* window, int& width, int& height) const;
    };
}
