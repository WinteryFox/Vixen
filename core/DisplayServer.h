#pragma once

#include <RenderingContextDriver.h>
#include <spdlog/spdlog.h>

#include "Monitor.h"
#include "RenderingDriver.h"
#include "VSyncMode.h"
#include "Window.h"
#include "WindowFlags.h"
#include "WindowMode.h"

namespace Vixen {
    class RenderingDevice;

    class DisplayServer final {
        RenderingDriver driver;

        RenderingContextDriver *renderingContextDriver;
        RenderingDevice *renderingDevice;

        Window *mainWindow = nullptr;

        Window *createWindow(
            const std::string &title,
            WindowMode mode,
            VSyncMode vsync,
            WindowFlags flags,
            glm::ivec2 resolution
        );

    public:
        DisplayServer(
            const std::string &applicationName,
            const glm::ivec3 &applicationVersion,
            RenderingDriver driver,
            WindowMode windowMode,
            VSyncMode vsyncMode,
            WindowFlags flags,
            glm::ivec2 resolution
        );

        DisplayServer(const DisplayServer &) = delete;

        DisplayServer &operator=(const DisplayServer &) = delete;

        DisplayServer(DisplayServer &&other) noexcept = delete;

        DisplayServer &operator=(DisplayServer &&other) noexcept = delete;

        ~DisplayServer();

        [[nodiscard]] Window *getMainWindow() const;

        [[nodiscard]] bool isFramebufferSizeChanged() const;

        /**
         * Has the current window been requested to close?
         * @return True if the window should close, false if not.
         */
        bool shouldClose(const Window *window);

        /**
         * Polls window events and processes them.
         * @return Returns true if the framebuffer size has been resized, false if not.
         */
        bool update(const Window *window);

        /**
         * Sets the visibility of the window.
         * @param visible True for visible, false for hidden.
         */
        void setVisible(const Window *window, bool visible);

        /**
         * Centers the window on the current monitor.
         */
        void center(const Window *window);

        void maximize(const Window *window);

        void setWindowedMode(const Window *window, WindowMode mode);

        void setVSyncMode(const Window *window, VSyncMode mode);

        [[nodiscard]] bool getWindowMonitor(const Window *window, Monitor &m);

        std::vector<Monitor> getMonitors();

        void getFramebufferSize(const Window *window, int &width, int &height);
    };
}
