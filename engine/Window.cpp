#include "Window.h"

Vixen::Editor::Window::Window(const std::string &title, uint32_t width, uint32_t height) {
    glfwSetErrorCallback([](int code, const char *message) {
        spdlog::error("[GLFW] {} ({})", message, code);
    });

    if (glfwInit() != GLFW_TRUE)
        throw std::runtime_error("Failed to create window");
#ifdef VULKAN_ENABLED
    if (glfwVulkanSupported() != GLFW_TRUE)
        throw std::runtime_error("Vulkan is not supported on this system, please try updating your drivers");
#endif

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_REFRESH_RATE, GLFW_DONT_CARE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window)
        glfwTerminate();

    const auto primary = glfwGetPrimaryMonitor();
    const auto mode = glfwGetVideoMode(primary);
    uint32_t x, y;
    glfwGetMonitorPos(primary, reinterpret_cast<int *>(&x), reinterpret_cast<int *>(&y));
    glfwSetWindowPos(window, x + mode->width / 2 - width / 2, y + mode->height / 2 - height / 2);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
}

Vixen::Editor::Window::~Window() {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Vixen::Editor::Window::shouldClose() const {
    return glfwWindowShouldClose(window) == GLFW_TRUE;
}

void Vixen::Editor::Window::update() {
    glfwPollEvents();
}

void Vixen::Editor::Window::setVisible(bool visible) {
    if (visible)
        glfwShowWindow(window);
    else
        glfwHideWindow(window);
}
