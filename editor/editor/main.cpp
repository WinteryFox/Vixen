#include <QApplication>
#include <QMainWindow>
#include "ui_mainwindow.h"  // CMake will automatically compile mainwindow.ui -> ui_mainwindow.h if it's included.

int main(int argc, char ** argv) {
    QApplication app {argc, argv};

    QMainWindow mainWindow;
    Ui::MainWindow mainWindowUi;

    mainWindowUi.setupUi(&mainWindow);
    mainWindow.show();

    return app.exec(); // NOLINT(readability-static-accessed-through-instance)
}
