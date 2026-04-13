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

    // Use standard native window chrome for reliable focus and mouse interaction.
    // The previous frameless + always-on-top combination could leave the window
    // visually active while native hit-testing still failed to deliver clicks.
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowMinimizeButtonHint | Qt.WindowCloseButtonHint

    function logState(prefix) {
        if (backend && backend.logDebugMessage) {
            backend.logDebugMessage(prefix + " visible=" + visible +
                                    " visibility=" + visibility +
                                    " active=" + active +
                                    " x=" + x + " y=" + y +
                                    " width=" + width + " height=" + height)
        }
    }

    onVisibleChanged: {
        console.log("[QML] Main window visibility changed:", visible, "visibility=", visibility)
        logState("mainWindow.onVisibleChanged")
    }

    onVisibilityChanged: {
        console.log("[QML] Main window visibility enum changed:", visibility)
        logState("mainWindow.onVisibilityChanged")
    }

    onActiveChanged: {
        console.log("[QML] Main window active changed:", active)
        logState("mainWindow.onActiveChanged")
    }

    onXChanged: {
        console.log("[QML] Main window X changed:", x)
        logState("mainWindow.onXChanged")
    }
    onYChanged: {
        console.log("[QML] Main window Y changed:", y)
        logState("mainWindow.onYChanged")
    }

    Connections {
        target: backend
        function onShowControlPanelRequested() {
            console.log("[QML] showControlPanelRequested before:", "visible=", mainWindow.visible, "visibility=", mainWindow.visibility, "active=", mainWindow.active)
            mainWindow.logState("before showControlPanelRequested")
            mainWindow.showNormal()
            mainWindow.raise()
            mainWindow.requestActivate()
            console.log("[QML] showControlPanelRequested after:", "visible=", mainWindow.visible, "visibility=", mainWindow.visibility, "active=", mainWindow.active)
            mainWindow.logState("after showControlPanelRequested")
        }
        function onToggleControlPanelRequested() {
            console.log("[QML] toggleControlPanelRequested:", "visible=", mainWindow.visible, "visibility=", mainWindow.visibility, "active=", mainWindow.active)
            mainWindow.logState("before toggleControlPanelRequested")
            if (mainWindow.visible && mainWindow.visibility !== Window.Minimized) {
                mainWindow.showMinimized()
                mainWindow.logState("after toggleControlPanelRequested minimize")
            } else {
                mainWindow.showNormal()
                mainWindow.raise()
                mainWindow.requestActivate()
                console.log("[QML] toggleControlPanelRequested restore:", "visible=", mainWindow.visible, "visibility=", mainWindow.visibility, "active=", mainWindow.active)
                mainWindow.logState("after toggleControlPanelRequested restore")
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
            onPressed: {
                console.log("[QML] Title bar press at", mouse.x, mouse.y, "window", mainWindow.x, mainWindow.y)
                mainWindow.logState("titleBar.onPressed mouseX=" + mouse.x + " mouseY=" + mouse.y)
                if (mouse.button === Qt.LeftButton)
                    mainWindow.startSystemMove()
            }
            onReleased: {
                mainWindow.logState("titleBar.onReleased mouseX=" + mouse.x + " mouseY=" + mouse.y)
            }
            onDoubleClicked: {
                console.log("[QML] Title bar double-click detected.")
                mainWindow.logState("titleBar.onDoubleClicked")
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
