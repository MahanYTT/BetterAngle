import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

Window {
    id: setupWindow
    width: 500
    height: 380
    visible: true
    title: "BetterAngle Pro Calibration"
    color: "#050508"
    
    // Step 3.3: Strict window flags
    flags: Qt.Window | Qt.WindowMinimizeButtonHint | Qt.WindowCloseButtonHint

    // Step 3.4: onClosing behavior
    onClosing: function(close_event) {
        // Prevent hiding to tray. If closed during setup, quit entirely.
        close_event.accepted = true;
        Qt.quit();
    }

    Rectangle {
        anchors.fill: parent
        color: "#050508"
        radius: 10
        border.color: "#1a1a25"
        border.width: 1

        Column {
            anchors.fill: parent
            anchors.margins: 30
            spacing: 20
            
            Text {
                text: "CALIBRATION WIZARD"
                color: "#00ffa3"
                font.pixelSize: 12
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: "Set In-Game Sensitivity"
                color: "white"
                font.pixelSize: 24
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: "Enter your Fortnite sensitivity below to ensure accuracy."
                color: "#888"
                font.pixelSize: 13
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Item {
                width: parent.width
                height: 80
                Row {
                    spacing: 20
                    anchors.horizontalCenter: parent.horizontalCenter
                    
                    Column {
                        spacing: 8
                        Text { text: "SENSITIVITY X"; color: "#666"; font.pixelSize: 11; font.bold: true }
                        TextField {
                            id: sensXInput
                            width: 180
                            height: 40
                            placeholderText: "0.05"
                            text: backend.sensX.toFixed(4)
                            color: "white"
                            background: Rectangle { color: "#1c1c2e"; radius: 4; border.color: parent.activeFocus ? "#00ffa3" : "#333" }
                        }
                    }

                    Column {
                        spacing: 8
                        Text { text: "SENSITIVITY Y"; color: "#666"; font.pixelSize: 11; font.bold: true }
                        TextField {
                            id: sensYInput
                            width: 180
                            height: 40
                            placeholderText: "0.05"
                            text: backend.sensY.toFixed(4)
                            color: "white"
                            background: Rectangle { color: "#1c1c2e"; radius: 4; border.color: parent.activeFocus ? "#00ffa3" : "#333" }
                        }
                    }
                }
            }

            Button {
                text: "SYNC WITH FORTNITE"
                width: parent.width
                height: 40
                onClicked: backend.syncWithFortnite()
                contentItem: Text { text: parent.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: parent.hovered ? "#00a382" : "#00cca3"; radius: 4 }
            }

            Item { width: 1; height: 10 } // Spacer

            Button {
                text: "FINISH SETUP"
                width: parent.width
                height: 44
                onClicked: {
                    backend.setSensX(parseFloat(sensXInput.text))
                    backend.setSensY(parseFloat(sensYInput.text))
                    backend.finishSetup()
                    setupWindow.visible = false
                }
                contentItem: Text { text: parent.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: "#0080ff"; radius: 4 }
            }
        }
    }
}
