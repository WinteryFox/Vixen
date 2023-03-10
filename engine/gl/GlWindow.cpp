#include "GlWindow.h"

namespace Vixen::Engine {
    GlWindow::GlWindow(const std::string &title, const int &width, const int &height, bool transparentFrameBuffer) :
            Vixen::Engine::Window(transparentFrameBuffer) {
        spdlog::trace("Creating new OpenGL window");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#ifdef DEBUG
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
            spdlog::debug("Enabling OpenGL debug extension");
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(glDebugCallback, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        }
#endif

        glfwDefaultWindowHints();
    }

    void GlWindow::clear() {
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GlWindow::swap() {
        glfwSwapBuffers(window);
    }
}
