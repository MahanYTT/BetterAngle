import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

Window {
    id: mainWindow
    width: 650
    height: 480
    x: Screen.width / 2 - width / 2
    y: Screen.height / 2 - height / 2
    visible: false
    title: qsTr("BetterAngle Pro Angle HUD")
    color: "#0a0a0f"

    // Frameless window style for a custom sleek look
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowSystemMenuHint | Qt.WindowMinimizeButtonHint | Qt.WindowStaysOnTopHint

    onVisibleChanged: {
        // No longer forcing crosshair state here to allow user preference to persist
    }

    Connections {
        target: backend
        function onShowControlPanelRequested() {
            mainWindow.showNormal()
            mainWindow.raise()
            mainWindow.requestActivate()
        }
        function onToggleControlPanelRequested() {
            if (mainWindow.visible && mainWindow.visibility !== Window.Minimized) {
                mainWindow.showMinimized()
            } else {
                mainWindow.showNormal()
                mainWindow.raise()
                mainWindow.requestActivate()
            }
        }
    }


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
            property point lastMousePos: Qt.point(0, 0)
            onPressed: lastMousePos = Qt.point(mouse.x, mouse.y)
            onPositionChanged: {
                var delta = Qt.point(mouse.x - lastMousePos.x, mouse.y - lastMousePos.y)
                mainWindow.x += delta.x
                mainWindow.y += delta.y
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
                onClicked: mainWindow.showMinimized()
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
