import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

Window {
    id: mainWindow
    width: 650
    height: 480
    minimumWidth: 500
    minimumHeight: 350
    maximumWidth: 1200
    maximumHeight: 800
    x: Screen.width / 2 - width / 2
    y: Screen.height / 2 - height / 2
    visible: false
    title: qsTr("BetterAngle Pro Angle HUD")
    color: "#0a0a0f"
    
    property bool isBooting: true
    
    Timer {
        id: bootTimer
        interval: 5000
        running: true
        repeat: false
        onTriggered: {
            mainWindow.isBooting = false
        }
    }

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
            onPressed: mainWindow.startSystemMove()
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
        enabled: !mainWindow.isBooting
        opacity: mainWindow.isBooting ? 0 : 1
        
        Behavior on opacity { NumberAnimation { duration: 500 } }
    }

    // Splash Screen Overlay
    Rectangle {
        id: splashScreen
        anchors.fill: parent
        color: "#0a0a0f"
        visible: mainWindow.isBooting
        z: 1000

        Image {
            id: splashImage
            source: "qrc:/assets/loading.png"
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            opacity: 0
            
            Component.onCompleted: fadeIn.start()
            
            NumberAnimation on opacity {
                id: fadeIn
                to: 1
                duration: 1000
            }
        }

        // Premium Loading Bar (Fire themed)
        Rectangle {
            id: loadingTrack
            width: parent.width * 0.8
            height: 6
            color: "#1a1a24"
            radius: 3
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40
            anchors.horizontalCenter: parent.horizontalCenter
            clip: true

            // The Progress Fill
            Rectangle {
                id: loadingFill
                height: parent.height
                radius: 3
                
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#ff4d00" } // Deep Fire Red
                    GradientStop { position: 0.5; color: "#ff8c00" } // Fire Orange
                    GradientStop { position: 1.0; color: "#ffcc00" } // Golden Glow
                }

                PropertyAnimation on width {
                    id: progressAnim
                    from: 0
                    to: 520
                    duration: 5000
                    running: mainWindow.isBooting
                }
            }

            // Lead edge glow for "fire" effect
            Rectangle {
                width: 20
                height: parent.height
                anchors.right: loadingFill.right
                visible: loadingFill.width > 0
                
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 1.0; color: "#ffffff" } // Bright spark at edge
                }
                opacity: 0.6
            }
        }
    }
}
