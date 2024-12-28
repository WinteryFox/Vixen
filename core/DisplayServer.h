#pragma once

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>

#include "Monitor.h"
#include "RenderingContext.h"

namespace Vixen {
    class RenderingDevice;

    class DisplayServer final {
    public:
        enum class RenderingDriver {
            Vulkan,
            D3D12
        };

        enum class WindowMode {
            Windowed,
            Minimized,
            Maximized,
            BorderlessFullscreen,
            ExclusiveFullscreen
        };

        enum class VSyncMode {
            Disabled,
            Enabled,
            Adaptive,
            Mailbox
        };

        enum class MouseMode {
            Visible,
            Hidden,
            Captured,
            Confined,
            ConfinedHidden
        };

        enum WindowFlags {
            WINDOW_FLAGS_RESIZABLE,
            WINDOW_FLAGS_BORDERLESS,
            WINDOW_FLAGS_TRANSPARENT,
            WINDOW_FLAGS_ALWAYS_ON_TOP
        };

        enum class Cursor {
            Arrow
        };

    private:
        RenderingDriver driver;

        glm::ivec2 resolution;

        GLFWwindow *mainWindow;

        bool framebufferSizeChanged = false;

        std::shared_ptr<RenderingContext> renderingContext;

        std::shared_ptr<RenderingDevice> renderingDevice;

        GLFWwindow *createWindow(
            WindowMode mode,
            VSyncMode vsync,
            WindowFlags flags,
            glm::ivec2 resolution
        );

    public:
        DisplayServer(
            RenderingDriver driver,
            WindowMode mode,
            VSyncMode vsync,
            WindowFlags flags,
            glm::ivec2 resolution
        );

        DisplayServer(const DisplayServer &) = delete;

        DisplayServer &operator=(const DisplayServer &) = delete;

        DisplayServer(DisplayServer &&other) noexcept;

        DisplayServer &operator=(DisplayServer &&other) noexcept;

        ~DisplayServer();

        [[nodiscard]] GLFWwindow *getWindow() const;

        [[nodiscard]] bool isFramebufferSizeChanged() const;

        /**
         * Has the current window been requested to close?
         * @return True if the window should close, false if not.
         */
        [[nodiscard]] bool shouldClose() const;

        /**
         * Polls window events and processes them.
         * @return Returns true if the framebuffer size has been resized, false if not.
         */
        bool update();

        /**
         * Sets the visibility of the window.
         * @param visible True for visible, false for hidden.
         */
        void setVisible(bool visible) const;

        /**
         * Centers the window on the current monitor.
         */
        void center() const;

        void maximize() const;

        void setWindowedMode(WindowMode mode) const;

        [[nodiscard]] bool getWindowMonitor(Monitor &m) const;

        static std::vector<Monitor> getMonitors();

        void getFramebufferSize(int &width, int &height) const;
    };
}
