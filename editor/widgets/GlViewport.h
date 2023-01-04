#pragma once

#include <spdlog/spdlog.h>
#include <QQuickItem>
#include "GlViewportRenderer.h"

namespace Vixen::Editor {
    class GlViewport : public QQuickItem {
    Q_OBJECT

        void releaseResources() override;

    public:
        explicit GlViewport(QQuickItem *parent = nullptr);

    public slots:

        void sync();

        void cleanup();

    private slots:
        void handleWindowChanged(QQuickWindow *window) const;

    private:
        std::unique_ptr<GlViewportRenderer> renderer;
    };
}
