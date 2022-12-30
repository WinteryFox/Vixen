#pragma once

#include <cstdint>
#include <string>
#include <GL/glew.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/ansicolor_sink-inl.h>
#include <fmt/color.h>
#include "../Window.h"

namespace Vixen::Engine::Gl {
    struct Window : Vixen::Engine::Window {
        Window(const std::string &title, const uint32_t &width, const uint32_t &height, bool transparentFrameBuffer);

        void clear();

        void swap();
    };
}

#ifdef DEBUG

static void APIENTRY glDebugOutput(
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
            src = "Window";
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
            src = "Other";
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
    spdlog::log(
            level,
            "[{}] {}",
            fmt::format(fmt::fg(fmt::terminal_color::magenta), "GL " + src),
            message
    );
}

#endif
