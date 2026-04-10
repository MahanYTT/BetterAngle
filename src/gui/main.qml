import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

Window {
    id: mainWindow
    width: 650
    height: 480
    visible: true
    title: qsTr("BetterAngle Pro Command Center")
    color: "#0a0a0f"

    // Frameless window style for a custom sleek look
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowSystemMenuHint | Qt.WindowMinimizeButtonHint

    Rectangle {
        id: titleBar
        width: parent.width
        height: 40
        color: "#181824"
        
        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 20
            text: "BetterAngle Pro"
            color: "#ffffff"
            font.bold: true
            font.pixelSize: 16
        }

        MouseArea {
            anchors.fill: parent
            property point clickPos: "0,0"

            onPressed: (mouse) => {
                clickPos = Qt.point(mouse.x, mouse.y)
            }

            onPositionChanged: (mouse) => {
                var deltaX = mouse.x - clickPos.x
                var deltaY = mouse.y - clickPos.y
                mainWindow.x += deltaX
                mainWindow.y += deltaY
            }
        }

        // Window Controls
        Row {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            Button {
                text: "—"
                width: 40
                height: 40
                background: Rectangle { color: parent.hovered ? "#303040" : "transparent" }
                contentItem: Text { text: parent.text; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: mainWindow.showMinimized()
            }
            Button {
                text: "✕"
                width: 40
                height: 40
                background: Rectangle { color: parent.hovered ? "#ff3333" : "transparent" }
                contentItem: Text { text: parent.text; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: backend.terminateApp()
            }
        }
    }

    Dashboard {
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
