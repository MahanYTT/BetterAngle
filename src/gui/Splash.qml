import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    id: splashWindow
    width: 480
    height: 300
    visible: true
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    Rectangle {
        anchors.fill: parent
        color: "#0a0a0f"
        radius: 12
        border.color: "#333"
        border.width: 1

        Column {
            anchors.centerIn: parent
            spacing: 20

            // Logo Gradient Circle
            Rectangle {
                width: 100
                height: 100
                radius: 50
                anchors.horizontalCenter: parent.horizontalCenter
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#00ffa3" }
                    GradientStop { position: 1.0; color: "#0080ff" }
                }

                Text {
                    anchors.centerIn: parent
                    text: "\x3E" // Arrow symbol
                    color: "white"
                    font.pixelSize: 40
                    font.bold: true
                }

                // Pulse effect
                Rectangle {
                    anchors.fill: parent
                    radius: 50
                    color: "transparent"
                    border.color: "#00ffa3"
                    border.width: 2
                    scale: 1.0
                    opacity: 1.0

                    SequentialAnimation on scale {
                        loops: Animation.Infinite
                        NumberAnimation { from: 1.0; to: 1.6; duration: 2000; easing.type: Easing.OutExpo }
                    }
                    SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        NumberAnimation { from: 1.0; to: 0.0; duration: 2000; easing.type: Easing.OutExpo }
                    }
                }
            }

            Column {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 4
                Text {
                    text: "BETTERANGLE PRO"
                    color: "white"
                    font.pixelSize: 28
                    font.bold: true
                    font.letterSpacing: 2
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: "STATE-OF-THE-ART ANGLE HUB"
                    color: "#00ffa3"
                    font.pixelSize: 10
                    font.bold: true
                    font.letterSpacing: 4
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            Rectangle {
                width: 200
                height: 4
                color: "#1a1a25"
                radius: 2
                anchors.horizontalCenter: parent.horizontalCenter

                Rectangle {
                    id: progressBar
                    width: 0
                    height: parent.height
                    color: "#00ffa3"
                    radius: 2

                    NumberAnimation on width {
                        from: 0; to: 200; duration: 1500; easing.type: Easing.InOutQuad
                    }
                }
            }
        }
    }

    Timer {
        interval: 1800
        running: true
        repeat: false
        onTriggered: {
            splashWindow.close()
            backend.requestShowControlPanel()
        }
    }
}
