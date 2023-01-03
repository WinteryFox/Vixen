#pragma once

#include <QOpenGLWidget>

namespace Vixen::Editor {
    class GlViewport : public QOpenGLWidget {
    public:
        explicit GlViewport(QWidget *parent);

    protected:
        void initializeGL() override;

        void resizeGL(int w, int h) override;

        void paintGL() override;
    };
}
