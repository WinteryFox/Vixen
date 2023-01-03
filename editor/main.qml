import QtQuick 2.12
import QtQuick.Shapes 1.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import QtQuick.Window 2.3

Window {
    id: window
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint
    width: 1080
    height: 720
    title: "Vixen Editor"

    function toggleMaximized() {
        if (window.visibility === Window.Maximized) {
            window.showNormal();
        } else {
            window.showMaximized();
        }
    }

    DragHandler {
        id: resizeHandler
        grabPermissions: TapHandler.TakeOverForbidden
        target: null
        onActiveChanged: if (active) {
            const p = resizeHandler.centroid.position;
            let e = 0;
            if (p.x / width < 0.10) { e |= Qt.LeftEdge }
            if (p.x / width > 0.90) { e |= Qt.RightEdge }
            if (p.y / height < 0.10) { e |= Qt.TopEdge }
            if (p.y / height > 0.90) { e |= Qt.BottomEdge }
            window.startSystemResize(e);
        }
    }

    Page {
        anchors.fill: parent
        header: ToolBar {
            contentHeight: toolButton.implicitHeight
            Item {
                anchors.fill: parent
                TapHandler {
                    onTapped: if (tapCount === 2) toggleMaximized()
                    gesturePolicy: TapHandler.DragThreshold
                }
                DragHandler {
                    grabPermissions: TapHandler.CanTakeOverFromAnything
                    onActiveChanged: if (active) { window.startSystemMove(); }
                }

                RowLayout {
                    anchors.right: parent.right
                    ToolButton {
                        text: "➖"
                        onClicked: window.showMinimized()
                    }
                    ToolButton {
                        text: window.visibility == Window.Maximized ? "🗗" : "🗖"
                        onClicked: window.toggleMaximized()
                    }
                    ToolButton {
                        text: "🗙"
                        onClicked: window.close()
                    }
                }
            }
        }


        Drawer {
            id: drawer
            width: window.width * 0.66
            height: window.height
            interactive: window.visibility !== Window.Windowed || position > 0
        }
    }
}
