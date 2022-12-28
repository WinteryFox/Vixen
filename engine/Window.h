#pragma once

#include <string>
#include <spdlog/spdlog.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace Vixen::Engine {
    class Window {
    protected:
        GLFWwindow *window;

        Window(const std::string &title, const uint32_t &width, const uint32_t &height);

        ~Window();

    public:
        [[nodiscard]] bool shouldClose() const;

        void update();

        void setVisible(bool visible);

        void center();
    };
}
