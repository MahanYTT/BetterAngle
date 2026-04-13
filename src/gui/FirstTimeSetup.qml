import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

Window {
    id: setupWindow
    width: 600
    height: 480
    visible: true
    title: "BetterAngle Pro Calibration"
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    Rectangle {
        id: rootRect
        anchors.fill: parent
        color: "#050508"
        radius: 12
        border.color: "#1a1a25"
        border.width: 1
        clip: true

        // ── Header (Draggable Area) ──────────────────────────────────
        Rectangle {
            id: header
            width: parent.width
            height: 50
            color: "#0a0a0f"
            radius: 12
            
            // Bottom cover for the radius
            Rectangle {
                width: parent.width; height: 12
                color: parent.color
                anchors.bottom: parent.bottom
            }

            Text {
                text: "BETTERANGLE PRO CALIBRATION"
                color: "#555"
                font.pixelSize: 10
                font.bold: true
                font.letterSpacing: 2
                anchors.centerIn: parent
            }

            MouseArea {
                anchors.fill: parent
                property point lastMousePos: Qt.point(0, 0)
                onPressed: (mouse) => { lastMousePos = Qt.point(mouse.x, mouse.y) }
                onPositionChanged: (mouse) => {
                    var delta = Qt.point(mouse.x - lastMousePos.x, mouse.y - lastMousePos.y)
                    setupWindow.x += delta.x
                    setupWindow.y += delta.y
                }
            }

            // Close button (Top Right)
            Text {
                text: "✕"
                color: "#444"
                font.pixelSize: 18
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: parent.color = "#ff4d4d"
                    onExited: parent.color = "#444"
                    onClicked: Qt.quit()
                }
            }
        }

        // ── Scrollable Content Area ─────────────────────────────────
        Flickable {
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            contentWidth: width
            contentHeight: contentColumn.height + 60
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: contentColumn
                width: parent.width - 80
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 24
                topPadding: 30

                Column {
                    spacing: 8
                    width: parent.width
                    Text {
                        text: "In-Game Sensitivity Sync"
                        color: "white"
                        font.pixelSize: 26
                        font.bold: true
                    }
                    Text {
                        text: "Sync with Fortnite or enter your values manually below."
                        color: "#888"
                        font.pixelSize: 14
                    }
                }

                // Fortnite Sync Section
                Rectangle {
                    width: parent.width
                    height: syncLayout.height + 40
                    color: "#0a0a0f"
                    radius: 8
                    border.color: "#1a1a25"

                    Column {
                        id: syncLayout
                        width: parent.width - 40
                        anchors.centerIn: parent
                        spacing: 16

                        Button {
                            id: syncBtn
                            text: "SYNC SENSITIVITY WITH FORTNITE"
                            width: parent.width
                            height: 48
                            onClicked: backend.syncWithFortnite()
                            contentItem: Text { 
                                text: parent.text
                                color: "white"
                                font.bold: true
                                font.letterSpacing: 1
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle { 
                                color: parent.hovered ? "#00cca3" : "#00ffa3"
                                radius: 6
                                opacity: parent.pressed ? 0.8 : 1.0
                                Text {
                                    anchors.centerIn: parent
                                    visible: parent.parent.hovered
                                    text: "AUTO-DETECT"
                                    color: "black"
                                    font.pixelSize: 10
                                    font.bold: true
                                    opacity: 0.3
                                }
                            }
                        }

                        Text {
                            text: backend.syncResult
                            color: backend.syncResult.indexOf("OK") !== -1 ? "#00ffa3" : "#ff4d4d"
                            font.pixelSize: 11
                            font.bold: true
                            wrapMode: Text.Wrap
                            width: parent.width
                        }
                    }
                }

                // Manual Input Section
                Row {
                    spacing: 20
                    width: parent.width

                    Column {
                        spacing: 10
                        width: (parent.width - 20) / 2
                        Text { text: "SENSITIVITY X"; color: "#666"; font.pixelSize: 11; font.bold: true; font.letterSpacing: 1 }
                        TextField {
                            id: sensXInput
                            width: parent.width
                            height: 44
                            placeholderText: "0.1"
                            text: backend.sensX.toFixed(1)
                            color: "white"
                            font.pixelSize: 16
                            font.family: "Consolas"
                            validator: DoubleValidator { bottom: 0.00001; top: 2.0; decimals: 1 }
                            background: Rectangle { 
                                color: "#13131a"
                                radius: 6
                                border.color: parent.activeFocus ? "#00ffa3" : "#1a1a25"
                                border.width: 1
                            }
                        }
                    }

                    Column {
                        spacing: 10
                        width: (parent.width - 20) / 2
                        Text { text: "SENSITIVITY Y"; color: "#666"; font.pixelSize: 11; font.bold: true; font.letterSpacing: 1 }
                        TextField {
                            id: sensYInput
                            width: parent.width
                            height: 44
                            placeholderText: "0.1"
                            text: backend.sensY.toFixed(1)
                            color: "white"
                            font.pixelSize: 16
                            font.family: "Consolas"
                            validator: DoubleValidator { bottom: 0.00001; top: 2.0; decimals: 1 }
                            background: Rectangle { 
                                color: "#13131a"
                                radius: 6
                                border.color: parent.activeFocus ? "#00ffa3" : "#1a1a25"
                                border.width: 1
                            }
                        }
                    }
                }

                Item { width: 1; height: 10 } // Spacer

                Button {
                    text: "FINISH SETUP & ENTER ENGINE"
                    width: parent.width
                    height: 54
                    onClicked: {
                        setupWindow.hide()
                        let sx = parseFloat(sensXInput.text)
                        let sy = parseFloat(sensYInput.text)
                        backend.sensX = isNaN(sx) ? 0.05 : sx
                        backend.sensY = isNaN(sy) ? 0.05 : sy
                        backend.finishSetup()
                    }
                    contentItem: Text { 
                        text: parent.text
                        color: "white"
                        font.pixelSize: 16
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { 
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#0080ff" }
                            GradientStop { position: 1.0; color: "#0052a3" }
                        }
                        radius: 8
                        border.color: "#3380ff"
                        opacity: parent.pressed ? 0.9 : 1.0
                    }
                }
            }
        }
    }
}
