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
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowSystemMenuHint | Qt.WindowMinimizeButtonHint

    onVisibleChanged: {
        // No longer forcing crosshair state here to allow user preference to persist
    }

    Connections {
        target: backend
        onShowControlPanelRequested: {
            if (mainWindow.visible) {
                mainWindow.hide()
            } else {
                mainWindow.show()
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
        
        Image {
            id: logo
            source: "qrc:/assets/logo.png"
            width: 24
            height: 24
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 12
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: logo.right
            anchors.leftMargin: 10
            text: "BetterAngle Pro"
            color: "#ffffff"
            font.bold: true
            font.pixelSize: 16
        }

        MouseArea {
            anchors.fill: parent
            property int lastGlobalX: 0
            property int lastGlobalY: 0
            onPressed: {
                lastGlobalX = mouse.globalX
                lastGlobalY = mouse.globalY
            }
            onPositionChanged: {
                if (pressed) {
                    var dx = mouse.globalX - lastGlobalX
                    var dy = mouse.globalY - lastGlobalY
                    mainWindow.x += dx
                    mainWindow.y += dy
                    lastGlobalX = mouse.globalX
                    lastGlobalY = mouse.globalY
                }
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
        }
    }

    Dashboard {
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
