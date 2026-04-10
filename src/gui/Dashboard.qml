import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    TabBar {
        id: bar
        width: parent.width
        background: Rectangle { color: "#0d0d12" }
        
        TabButton {
            text: qsTr("GENERAL")
            contentItem: Text { text: parent.text; color: parent.checked ? "#00ffcc" : "#888"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { color: parent.checked ? "#1a1a2e" : "transparent" }
        }
        TabButton {
            text: qsTr("CROSSHAIR")
            contentItem: Text { text: parent.text; color: parent.checked ? "#00ffcc" : "#888"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { color: parent.checked ? "#1a1a2e" : "transparent" }
        }
        TabButton {
            text: qsTr("COLORS")
            contentItem: Text { text: parent.text; color: parent.checked ? "#00ffcc" : "#888"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { color: parent.checked ? "#1a1a2e" : "transparent" }
        }
        TabButton {
            text: qsTr("DEBUG")
            contentItem: Text { text: parent.text; color: parent.checked ? "#00ffcc" : "#888"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { color: parent.checked ? "#1a1a2e" : "transparent" }
        }
        TabButton {
            text: qsTr("UPDATES")
            contentItem: Text { text: parent.text; color: parent.checked ? "#00ffcc" : "#888"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { color: parent.checked ? "#1a1a2e" : "transparent" }
        }
    }

    StackLayout {
        width: parent.width
        anchors.top: bar.bottom
        anchors.bottom: parent.bottom
        currentIndex: bar.currentIndex

        // GENERAL
        Rectangle {
            color: "#0d0d12"
            Column {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 15

                Text { text: "MANUAL SENSITIVITY"; color: "#666"; font.pixelSize: 12 }
                
                RowLayout {
                    spacing: 20
                    ColumnLayout {
                        Text { text: "Fortnite Sens X"; color: "white" }
                        TextField { 
                            text: backend.sensX.toFixed(4)
                            color: "white"
                            background: Rectangle { color: "#222"; radius: 4 }
                            onEditingFinished: backend.sensX = parseFloat(text)
                        }
                    }
                    ColumnLayout {
                        Text { text: "Fortnite Sens Y"; color: "white" }
                        TextField { 
                            text: backend.sensY.toFixed(4)
                            color: "white"
                            background: Rectangle { color: "#222"; radius: 4 }
                            onEditingFinished: backend.sensY = parseFloat(text)
                        }
                    }
                }

                Button {
                    text: "SYNC SENSITIVITY WITH FORTNITE"
                    width: parent.width
                    height: 40
                    contentItem: Text { text: parent.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { color: parent.hovered ? "#00a382" : "#00cca3"; radius: 4 }
                    onClicked: backend.syncWithFortnite()
                }

                Text {
                    text: backend.syncResult
                    color: backend.syncResult.includes("OK") ? "#00cca3" : "#ff3333"
                    font.bold: true
                }
            }
        }

        // CROSSHAIR
        Rectangle {
            color: "#0d0d12"
            Column {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 15

                Button {
                    text: backend.crosshairOn ? "CROSSHAIR: ON" : "CROSSHAIR: OFF"
                    width: parent.width
                    height: 40
                    contentItem: Text { text: parent.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { color: parent.hovered ? "#444" : "#222"; radius: 4 }
                    onClicked: backend.crosshairOn = !backend.crosshairOn
                }

                RowLayout {
                    Text { text: "Thickness"; color: "white"; Layout.preferredWidth: 80 }
                    Slider { 
                        from: 1.0; to: 10.0; value: backend.crossThickness
                        onValueChanged: backend.crossThickness = value
                    }
                }
                RowLayout {
                    Text { text: "Offset X"; color: "white"; Layout.preferredWidth: 80 }
                    Slider { 
                        from: -500.0; to: 500.0; value: backend.crossOffsetX
                        onValueChanged: backend.crossOffsetX = value
                    }
                }
                RowLayout {
                    Text { text: "Offset Y"; color: "white"; Layout.preferredWidth: 80 }
                    Slider { 
                        from: -500.0; to: 500.0; value: backend.crossOffsetY
                        onValueChanged: backend.crossOffsetY = value
                    }
                }

                CheckBox {
                    text: "Pulse Animation"
                    checked: backend.crossPulse
                    onCheckedChanged: backend.crossPulse = checked
                    contentItem: Text { text: parent.text; color: "white"; leftPadding: parent.indicator.width + 10; verticalAlignment: Text.AlignVCenter }
                }
            }
        }

        // COLORS
        Rectangle {
            color: "#0d0d12"
            Column {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 15

                Text { text: "Tolerance (color match ±)"; color: "white" }
                Slider {
                    width: parent.width
                    from: 0; to: 120; value: backend.tolerance
                    onValueChanged: backend.tolerance = Math.round(value)
                }
                Text { text: "Value: " + backend.tolerance; color: "#aaa" }
            }
        }

        // DEBUG
        Rectangle {
            color: "#0d0d12"
            Column {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 10

                CheckBox {
                    text: "Debug Overlay (Ctrl+9)"
                    checked: backend.debugMode
                    onCheckedChanged: backend.debugMode = checked
                    contentItem: Text { text: parent.text; color: "white"; leftPadding: parent.indicator.width + 10 }
                }
                CheckBox {
                    text: "Force Diving State"
                    checked: backend.forceDiving
                    onCheckedChanged: backend.forceDiving = checked
                    contentItem: Text { text: parent.text; color: "white"; leftPadding: parent.indicator.width + 10 }
                }

                Text { text: "DIVE THRESHOLDS"; color: "#666"; font.pixelSize: 12; topPadding: 10 }
                
                Text { text: "Glide Threshold"; color: "white" }
                Slider { width: parent.width; from: 0.01; to: 0.5; value: backend.glideThreshold; onValueChanged: backend.glideThreshold = value }

                Text { text: "Freefall Threshold"; color: "white" }
                Slider { width: parent.width; from: 0.01; to: 0.5; value: backend.freefallThreshold; onValueChanged: backend.freefallThreshold = value }

                Button {
                    text: "SAVE THRESHOLDS"
                    width: parent.width
                    height: 40
                    contentItem: Text { text: parent.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { color: parent.hovered ? "#444" : "#222"; radius: 4 }
                    onClicked: backend.saveThresholds()
                }
            }
        }

        // UPDATES
        Rectangle {
            color: "#0d0d12"
            Column {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 15

                Text { text: "Version: " + backend.versionStr; color: "white"; font.pixelSize: 16 }
                
                Text { 
                    text: "Latest: " + backend.latestVersion
                    color: "#aaa"
                    visible: backend.latestVersion !== ""
                }

                Button {
                    text: backend.updateAvailable ? "DOWNLOAD UPDATE NOW" : "CHECK FOR UPDATES"
                    width: parent.width
                    height: 40
                    contentItem: Text { text: parent.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    background: Rectangle { color: parent.hovered ? "#444" : "#222"; radius: 4 }
                    onClicked: backend.updateAvailable ? backend.downloadUpdate() : backend.checkForUpdates()
                }

                Text {
                    text: "Downloading update..."
                    color: "#00ccff"
                    visible: backend.isDownloading
                }
            }
        }
    }
}
