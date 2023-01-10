import QtQuick 2.15

Item {
    id: dock
    property string title
    property color backgroundColor: "#2D2D30"
    default property alias content: placeholder.data

    Rectangle {
        id: background
        anchors.fill: parent
    }

    Item {
        id: bar

        Text {
            text: "Propeties"

            anchors: {
                top: parent.top
                bottom: parent.bottom
                left: parent.left
                right: parent.right
            }
        }
    }

    Item {
        id: placeholder

        anchors {
            top: bar.bottom
            bottom: parent.bottom;
            left: parent.left;
            right: parent.right;
        }

        Rectangle {
            color: dock.backgroundColor
            anchors.fill: parent
        }
    }
}
