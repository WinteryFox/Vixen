#pragma once

#include <string>
#include <spdlog/spdlog.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include "Monitor.h"

namespace Vixen::Engine {
    class Window {
    public:
        enum class Mode {
            WINDOWED,
            WINDOWED_FULLSCREEN,
            FULLSCREEN
        };

    protected:
        GLFWwindow *window;

        std::unordered_map<GLFWmonitor *, Monitor> monitors;

        glm::vec4 clearColor{0.0f, 0.0f, 0.0f, 1.0f};

        Window(const std::string &title, const uint32_t &width, const uint32_t &height, bool transparentFrameBuffer);

        ~Window();

    public:
        [[nodiscard]] bool shouldClose() const;

        void update();

        void setVisible(bool visible);

        void center();

        void setWindowedMode(Mode mode, int width, int height, int x, int y, int refreshRate);

        std::unique_ptr<Monitor> getMonitor();

        std::unordered_map<GLFWmonitor *, Monitor> getMonitors();

        void setClearColor(float r, float g, float b, float a);
    };
}
