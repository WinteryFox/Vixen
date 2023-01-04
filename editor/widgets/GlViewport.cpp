#include "GlViewport.h"

namespace Vixen::Editor {
    GlViewport::GlViewport(QQuickItem *parent) : QQuickItem(parent), renderer(nullptr) {
        connect(this, &QQuickItem::windowChanged, this, &GlViewport::handleWindowChanged);
    }

    void GlViewport::handleWindowChanged(QQuickWindow *window) const {
        if (window) {
            connect(window, &QQuickWindow::beforeSynchronizing, this, &GlViewport::sync, Qt::DirectConnection);
            connect(window, &QQuickWindow::sceneGraphInvalidated, this, &GlViewport::cleanup, Qt::DirectConnection);

            window->setColor(Qt::black);
        }
    }

    void GlViewport::sync() {
        if (!renderer) {
            renderer = std::make_unique<GlViewportRenderer>();
            connect(window(), &QQuickWindow::beforeRendering, renderer.get(), &GlViewportRenderer::init,
                    Qt::DirectConnection);
            connect(window(), &QQuickWindow::beforeRenderPassRecording, renderer.get(), &GlViewportRenderer::paint,
                    Qt::DirectConnection);
        }

        renderer->setViewportSize(window()->size() * window()->devicePixelRatio());
        renderer->setWindow(window());
    }

    void GlViewport::cleanup() {
        renderer = nullptr;
    }

    void GlViewport::releaseResources() {
        //window()->scheduleRenderJob(new CleanupJob(renderer), QQuickWindow::BeforeSynchronizingStage);
        renderer = nullptr;
    }
}
