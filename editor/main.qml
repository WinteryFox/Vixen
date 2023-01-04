import QtQuick 2.12
import QtQuick.Shapes 1.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import QtQuick.Window 2.3

ApplicationWindow {
    id: window
    visible: true
//!    flags: Qt.Window | Qt.FramelessWindowHint
    width: 1150
    height: 800
    minimumWidth: 1050
    minimumHeight: 500
    title: "Vixen Editor"

    function toggleMaximized() {
        if (window.visibility === Window.Maximized) {
            window.showNormal();
        } else {
            window.showMaximized();
        }
    }

    Viewport {

    }
}
