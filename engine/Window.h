#pragma once

#include <string>
#include <spdlog/spdlog.h>
#include <GLFW/glfw3.h>
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

        glm::vec4 clearColor{0.0f, 0.0f, 0.0f, 1.0f};

        explicit Window(bool transparentFrameBuffer);

        ~Window();

    public:
        [[nodiscard]] bool shouldClose() const;

        static void update();

        void setVisible(bool visible) const;

        void center() const;

        void setWindowedMode(Mode mode) const;

        std::unique_ptr<Monitor> getMonitor() const;

        std::unordered_map<GLFWmonitor *, Monitor> getMonitors() const;

        void setClearColor(float r, float g, float b, float a);

        void getFramebufferSize(int &width, int &height);
    };
}
