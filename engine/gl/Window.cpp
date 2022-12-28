#include "Window.h"

namespace Vixen::Engine::Gl {
    Window::Window(const std::string &title, const uint32_t &width, const uint32_t &height) :
            Vixen::Engine::Window(title, width, height) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#ifdef DEBUG
        if (GLEW_ARB_debug_output)
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

        window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!window) {
            spdlog::error("Failed to create window");
            glfwTerminate();
            throw std::runtime_error("Failed to create window");
        }
        glfwMakeContextCurrent(window);

        glewExperimental = GL_TRUE;
        auto result = glewInit();
        if (GLEW_OK != result) {
            spdlog::error("Failed to initialize glew: {}", reinterpret_cast<const char *>(glewGetErrorString(result)));
            throw std::runtime_error("Failed to initialize glew");
        }

#ifdef DEBUG
        if (GLEW_ARB_debug_output) {
            spdlog::info("Enabling OpenGL debug extension");
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(glDebugOutput, nullptr);
            glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        }
#endif

        glfwDefaultWindowHints();
    }
}
