#pragma once

#include <cstdint>
#include <string>
#include <GL/glew.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <fmt/color.h>
#include "../Window.h"

#ifdef DEBUG



#endif

namespace Vixen::Engine {
#ifdef DEBUG
    static void APIENTRY glDebugCallback(
            GLenum source,
            GLenum type,
            GLuint id,
            GLenum severity,
            GLsizei length,
            const GLchar *message,
            const void *userParam
    ) {
        std::string src;
        switch (source) {
            case GL_DEBUG_SOURCE_API:
                src = "API";
                break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                src = "GlWindow";
                break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER:
                src = "Shader";
                break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:
                src = "Third Party";
                break;
            case GL_DEBUG_SOURCE_APPLICATION:
                src = "Application";
                break;
            default:
                src = "Unknown";
                break;
        }

        std::string typ;
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:
                typ = "ERROR";
                break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                typ = "DEPRECATED BEHAVIOUR";
                break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                typ = "UNDEFINED BEHAVIOUR";
                break;
            case GL_DEBUG_TYPE_PORTABILITY:
                typ = "PORTABILITY";
                break;
            case GL_DEBUG_TYPE_PERFORMANCE:
                typ = "PERFORMANCE";
                break;
            case GL_DEBUG_TYPE_MARKER:
                typ = "MARKER";
                break;
            case GL_DEBUG_TYPE_PUSH_GROUP:
                typ = "PUSH GROUP";
                break;
            case GL_DEBUG_TYPE_POP_GROUP:
                typ = "POP GROUP";
                break;
            case GL_DEBUG_TYPE_OTHER:
                typ = "OTHER";
                break;
            default:
                typ = "UNKNOWN";
                break;
        }

        spdlog::level::level_enum level;
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
                level = spdlog::level::err;
                break;
            case GL_DEBUG_SEVERITY_MEDIUM:
            case GL_DEBUG_SEVERITY_LOW:
                level = spdlog::level::warn;
                break;
            default:
                level = spdlog::level::trace;
                break;
        }
        auto glDebugLogger = spdlog::get("OpenGL");
        if (glDebugLogger == nullptr)
            glDebugLogger = spdlog::stdout_color_mt("OpenGL");
        glDebugLogger->log(
                level,
                "[{} | {}] ({}) {}",
                fmt::format(fmt::fg(fmt::terminal_color::magenta), src),
                fmt::format(fmt::fg(fmt::terminal_color::blue), typ),
                id,
                message
        );
    }

#endif

    class GlWindow : public Window {
    public:
        GlWindow(const std::string &title, const int &width, const int &height, bool transparentFrameBuffer);

        void clear();

        void swap();
    };
}
