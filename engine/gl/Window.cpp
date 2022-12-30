#include "Window.h"

namespace Vixen::Engine::Gl {
    Window::Window(const std::string &title, const uint32_t &width, const uint32_t &height, bool transparentFrameBuffer) :
            Vixen::Engine::Window(title, width, height, transparentFrameBuffer) {
        spdlog::trace("Creating new OpenGL window");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
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

    void Window::clear() {
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Window::swap() {
        glfwSwapBuffers(window);
    }
}
