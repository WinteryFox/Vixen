#include "GlViewport.h"

namespace Vixen::Editor {
    GlViewport::GlViewport(QWidget *parent) : QOpenGLWidget(parent) {}

    void GlViewport::initializeGL() {
        QOpenGLWidget::initializeGL();
    }

    void GlViewport::resizeGL(int w, int h) {
        QOpenGLWidget::resizeGL(w, h);
    }

    void GlViewport::paintGL() {
        QOpenGLWidget::paintGL();
    }
}
