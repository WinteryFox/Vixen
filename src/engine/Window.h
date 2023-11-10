#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <string>
#include <spdlog/spdlog.h>
#include <iostream>
#include <glm/glm.hpp>
#include "Monitor.h"

namespace Vixen {
    class Window {
    public:
        enum class Mode {
            FULLSCREEN,
            BORDERLESS_FULLSCREEN,
            WINDOWED,
        };

    protected:
        GLFWmonitor *monitor;

        GLFWwindow *window;

        std::unordered_map<GLFWmonitor *, Monitor> monitors;

    public:
        Window(
                const std::string &title,
                const uint32_t &width,
                const uint32_t &height,
                bool transparentFrameBuffer
        );

        ~Window();

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

        void setWindowedMode(Mode mode) const;

        std::unique_ptr<Monitor> getMonitor() const;

        std::unordered_map<GLFWmonitor *, Monitor> getMonitors() const;

        void getFramebufferSize(int &width, int &height);

    private:
        bool framebufferSizeChanged = false;
    };
}
