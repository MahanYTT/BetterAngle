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

        // ─── GENERAL ────────────────────────────────────────────────
        Rectangle {
            color: "#0d0d12"
            Flickable {
                anchors.fill: parent
                contentHeight: genCol.implicitHeight + 40
                clip: true
                Column {
                    id: genCol
                    anchors { left: parent.left; right: parent.right; top: parent.top; margins: 20 }
                    spacing: 14

                    Text { text: "MANUAL SENSITIVITY"; color: "#666"; font.pixelSize: 11; font.bold: true }

                    // Sens X
                    Column {
                        spacing: 4
                        width: parent.width
                        Text { text: "Fortnite Sens X"; color: "#aaa"; font.pixelSize: 12 }
                        TextField {
                            id: sensXField
                            width: parent.width
                            // Re-read when profile changes so value always shows on startup
                            text: backend.sensX.toFixed(4)
                            color: "white"
                            background: Rectangle { color: "#1c1c2e"; radius: 4; border.color: "#333"; border.width: 1 }
                            onEditingFinished: backend.sensX = parseFloat(text)
                            Connections {
                                target: backend
                                function onProfileChanged() { sensXField.text = backend.sensX.toFixed(4) }
                            }
                        }
                    }

                    // Sens Y
                    Column {
                        spacing: 4
                        width: parent.width
                        Text { text: "Fortnite Sens Y"; color: "#aaa"; font.pixelSize: 12 }
                        TextField {
                            id: sensYField
                            width: parent.width
                            text: backend.sensY.toFixed(4)
                            color: "white"
                            background: Rectangle { color: "#1c1c2e"; radius: 4; border.color: "#333"; border.width: 1 }
                            onEditingFinished: backend.sensY = parseFloat(text)
                            Connections {
                                target: backend
                                function onProfileChanged() { sensYField.text = backend.sensY.toFixed(4) }
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
                        visible: backend.syncResult !== ""
                    }

                    Button {
                        text: "QUIT APP"
                        width: parent.width
                        height: 40
                        contentItem: Text { text: parent.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: parent.hovered ? "#ff4c4c" : "#e63939"; radius: 4 }
                        onClicked: backend.terminateApp()
                    }
                }
            }
        }

        // ─── CROSSHAIR ──────────────────────────────────────────────
        Rectangle {
            color: "#0d0d12"
            Flickable {
                anchors.fill: parent
                contentHeight: crossCol.implicitHeight + 40
                clip: true
                Column {
                    id: crossCol
                    anchors { left: parent.left; right: parent.right; top: parent.top; margins: 16 }
                    spacing: 12

                    // Toggle
                    Button {
                        text: backend.crosshairOn ? "CROSSHAIR: ON" : "CROSSHAIR: OFF"
                        width: parent.width
                        height: 38
                        contentItem: Text { text: parent.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: backend.crosshairOn ? (parent.hovered ? "#00a382" : "#00cca3") : (parent.hovered ? "#555" : "#333"); radius: 4 }
                        onClicked: backend.crosshairOn = !backend.crosshairOn
                    }

                    // Thickness
                    Column { spacing: 4; width: parent.width
                        Text { text: "Line Thickness: " + backend.crossThickness.toFixed(1) + " px"; color: "white"; font.pixelSize: 12 }
                        Slider {
                            width: parent.width
                            from: 1.0; to: 10.0
                            value: backend.crossThickness
                            onMoved: backend.crossThickness = value
                        }
                    }

                    // Color
                    Text { text: "CROSSHAIR COLOUR"; color: "#666"; font.pixelSize: 11; font.bold: true }
                    Row {
                        spacing: 10; width: parent.width
                        Rectangle {
                            width: 36; height: 36; radius: 4
                            color: Qt.rgba(backend.crossColor.r, backend.crossColor.g, backend.crossColor.b, 1)
                            border.color: "#555"; border.width: 1
                        }
                        Column {
                            spacing: 4
                            Row { spacing: 6
                                Text { text: "R"; color: "#ff6666"; width: 14; verticalAlignment: Text.AlignVCenter }
                                Slider { id: slR; from: 0; to: 255; width: 160; value: backend.crossColor.r * 255
                                    onMoved: backend.crossColor = Qt.rgba(value/255, backend.crossColor.g, backend.crossColor.b, 1) }
                                Text { text: Math.round(slR.value); color: "#aaa"; width: 28 }
                            }
                            Row { spacing: 6
                                Text { text: "G"; color: "#66ff88"; width: 14; verticalAlignment: Text.AlignVCenter }
                                Slider { id: slG; from: 0; to: 255; width: 160; value: backend.crossColor.g * 255
                                    onMoved: backend.crossColor = Qt.rgba(backend.crossColor.r, value/255, backend.crossColor.b, 1) }
                                Text { text: Math.round(slG.value); color: "#aaa"; width: 28 }
                            }
                            Row { spacing: 6
                                Text { text: "B"; color: "#6699ff"; width: 14; verticalAlignment: Text.AlignVCenter }
                                Slider { id: slB; from: 0; to: 255; width: 160; value: backend.crossColor.b * 255
                                    onMoved: backend.crossColor = Qt.rgba(backend.crossColor.r, backend.crossColor.g, value/255, 1) }
                                Text { text: Math.round(slB.value); color: "#aaa"; width: 28 }
                            }
                        }
                    }

                    // Pulse
                    CheckBox {
                        text: "Pulse Animation"
                        checked: backend.crossPulse
                        onToggled: backend.crossPulse = checked
                        contentItem: Text { text: parent.text; color: "white"; leftPadding: parent.indicator.width + 10; verticalAlignment: Text.AlignVCenter }
                    }

                    // Fine Position  
                    Text { text: "FINE POSITION"; color: "#666"; font.pixelSize: 11; font.bold: true }
                    Row {
                        spacing: 6; width: parent.width
                        Text { text: "X: " + backend.crossOffsetX.toFixed(1); color: "white"; verticalAlignment: Text.AlignVCenter; width: 80 }
                        Button { text: "X −0.5"; width: 70; height: 30
                            contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            background: Rectangle { color: parent.hovered ? "#555" : "#333"; radius: 4 }
                            onClicked: backend.crossOffsetX = backend.crossOffsetX - 0.5 }
                        Button { text: "X +0.5"; width: 70; height: 30
                            contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            background: Rectangle { color: parent.hovered ? "#555" : "#333"; radius: 4 }
                            onClicked: backend.crossOffsetX = backend.crossOffsetX + 0.5 }
                    }
                    Row {
                        spacing: 6; width: parent.width
                        Text { text: "Y: " + backend.crossOffsetY.toFixed(1); color: "white"; verticalAlignment: Text.AlignVCenter; width: 80 }
                        Button { text: "Y −0.5"; width: 70; height: 30
                            contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            background: Rectangle { color: parent.hovered ? "#555" : "#333"; radius: 4 }
                            onClicked: backend.crossOffsetY = backend.crossOffsetY - 0.5 }
                        Button { text: "Y +0.5"; width: 70; height: 30
                            contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            background: Rectangle { color: parent.hovered ? "#555" : "#333"; radius: 4 }
                            onClicked: backend.crossOffsetY = backend.crossOffsetY + 0.5 }
                    }
                    Button { text: "RESET CENTER"; width: parent.width; height: 30
                        contentItem: Text { text: parent.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: parent.hovered ? "#6644aa" : "#4a3080"; radius: 4 }
                        onClicked: { backend.crossOffsetX = 0; backend.crossOffsetY = 0 } }

                    // Saved Positions
                    Text { text: "SAVED POSITIONS"; color: "#666"; font.pixelSize: 11; font.bold: true }

                    Row {
                        spacing: 8; width: parent.width
                        TextField {
                            id: presetNameField
                            width: parent.width - 110
                            placeholderText: "Position name..."
                            color: "white"
                            background: Rectangle { color: "#1c1c2e"; radius: 4; border.color: "#333"; border.width: 1 }
                        }
                        Button {
                            text: "SAVE"
                            width: 96; height: 34
                            contentItem: Text { text: parent.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            background: Rectangle { color: parent.hovered ? "#3375ee" : "#224ecc"; radius: 4 }
                            onClicked: {
                                if (presetNameField.text.trim() !== "") {
                                    backend.saveCrosshairPreset(presetNameField.text.trim())
                                    presetNameField.text = ""
                                    presetList.model = backend.crosshairPresetNames()
                                }
                            }
                        }
                    }

                    // Preset list
                    Column {
                        id: presetListContainer
                        width: parent.width
                        spacing: 4

                        Connections {
                            target: backend
                            function onCrosshairPresetsChanged() { presetList.model = backend.crosshairPresetNames() }
                        }

                        ListView {
                            id: presetList
                            width: parent.width
                            height: Math.min(model.length * 38, 160)
                            model: backend.crosshairPresetNames()
                            clip: true
                            delegate: Rectangle {
                                width: presetList.width
                                height: 34
                                color: "#1a1a2e"
                                radius: 4
                                Row {
                                    anchors { fill: parent; leftMargin: 8; rightMargin: 4 }
                                    spacing: 6
                                    Text {
                                        text: modelData
                                        color: "white"
                                        font.pixelSize: 11
                                        verticalAlignment: Text.AlignVCenter
                                        width: parent.width - 80
                                        elide: Text.ElideRight
                                        height: parent.height
                                    }
                                    Button { text: "Load"; width: 46; height: 26
                                        contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        background: Rectangle { color: parent.hovered ? "#3a9e6e" : "#2a7a54"; radius: 3 }
                                        onClicked: { backend.loadCrosshairPreset(index); presetList.model = backend.crosshairPresetNames() }
                                    }
                                    Button { text: "✕"; width: 26; height: 26
                                        contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        background: Rectangle { color: parent.hovered ? "#cc3333" : "#882222"; radius: 3 }
                                        onClicked: { backend.deleteCrosshairPreset(index); presetList.model = backend.crosshairPresetNames() }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // ─── COLORS ─────────────────────────────────────────────────
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

        // ─── DEBUG ──────────────────────────────────────────────────
        Rectangle {
            color: "#0d0d12"
            Column {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 10

                CheckBox {
                    text: "Debug Overlay (Ctrl+9)"
                    checked: backend.debugMode
                    onToggled: backend.debugMode = checked
                    contentItem: Text { text: parent.text; color: "white"; leftPadding: parent.indicator.width + 10 }
                }
                CheckBox {
                    text: "Force Diving State"
                    checked: backend.forceDiving
                    onToggled: backend.forceDiving = checked
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

        // ─── UPDATES ────────────────────────────────────────────────
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
