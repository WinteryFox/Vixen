#include <spdlog/spdlog.h>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include "widgets/GlViewport.h"

using namespace Vixen::Editor;

int main(int argc, char **argv) {
    spdlog::set_level(spdlog::level::trace);

    QGuiApplication app{argc, argv};
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QCoreApplication::setApplicationName("Vixen Editor");
    QCoreApplication::setOrganizationName("Vixen");

    qmlRegisterType<GlViewport>("Vixen", 1, 0, "GlViewport");

    QQmlApplicationEngine engine;
    engine.load(u"qrc:/editor/ui/main.qml"_qs);

    return QGuiApplication::exec();

    /*QGuiApplication app{argc, argv};
    QWindow window = QWindow();
    window.setTitle(QString::fromStdString("Vixen Vk"));
    window.resize(1150, 800);
    window.setMinimumSize(QSize(1050, 500));
    window.setVisible(true);

    return app.exec();*/
}
