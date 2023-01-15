#pragma once

#include "gl/GlShaderModule.h"
#include "gl/GlShaderProgram.h"
#include "gl/GlBuffer.h"
#include "gl/GlVertexArrayObject.h"
#include "gl/GlWindow.h"

#include <QObject>
#include <QQuickWindow>
#include <QSize>
#include <glm/glm.hpp>
#include <fstream>

using namespace Vixen::Engine::Gl;

namespace Vixen::Editor {
    class GlViewportRenderer : public QObject {
    Q_OBJECT

        std::vector<glm::vec3> vertices = {
                {0.5f,  0.5f,  0.0f},  // top right
                {0.5f,  -0.5f, 0.0f},  // bottom right
                {-0.5f, -0.5f, 0.0f},  // bottom left
                {-0.5f, 0.5f,  0.0f}   // top left
        };

        std::vector<std::uint32_t> indices = {  // note that we start from 0!
                0, 1, 3,   // first triangle
                1, 2, 3    // second triangle
        };

        QSize viewportSize;

        QQuickWindow *window = nullptr;

        std::shared_ptr<GlShaderProgram> program;

        std::shared_ptr<GlBuffer> vbo;

        std::shared_ptr<GlVertexArrayObject> vao;

    public:
        void setViewportSize(const QSize &size);

        void setWindow(QQuickWindow *win);

    public slots:

        void init();

        void paint();
    };
}
