#include <QApplication>
#include <QMainWindow>
#include <QUrl>
#include <QQmlApplicationEngine>
#include "ui_mainwindow.h"
#include "widgets/GlViewport.h"

using namespace Vixen::Editor;

int main(int argc, char **argv) {
    QGuiApplication app{argc, argv};
    QQmlApplicationEngine engine;
    engine.load(u"qrc:/resources/main.qml"_qs);

    /*QMainWindow mainWindow;
    Vixen::Editor::Ui::MainWindow window;
    mainWindow.setWindowFlag(Qt::FramelessWindowHint);
    window.setupUi(&mainWindow);

    auto viewport = GlViewport(nullptr);
    mainWindow.setCentralWidget(&viewport);

    mainWindow.show();
    mainWindow.setWindowState(Qt::WindowMinimized);*/

    return app.exec(); // NOLINT(readability-static-accessed-through-instance)
}
