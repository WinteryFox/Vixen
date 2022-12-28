#pragma once

#define GLFW_INCLUDE_NONE

#include <string>
#include <spdlog/spdlog.h>
#include <GLFW/glfw3.h>

namespace Vixen::Editor {
    class Window {
        GLFWwindow *window;

    public:
        Window(const std::string &title, uint32_t width, uint32_t height);

        ~Window();

        [[nodiscard]] bool shouldClose() const;

        static void update();

        void setVisible(bool visible);
    };
}
