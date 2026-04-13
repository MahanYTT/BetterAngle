import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

Window {
    id: splashWindow
    width: 760
    height: 460
    x: Screen.width / 2 - width / 2
    y: Screen.height / 2 - height / 2
    visible: true
    title: qsTr("BetterAngle Splash")
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    property int dotPhase: 0
    property real visualProgress: 0.08

    Component.onCompleted: {
        console.log("[QML] Splash redesign loaded successfully.")
    }

    onVisibleChanged: {
        console.log("[QML] Splash visibility changed:", visible)
    }

    Connections {
        target: backend
        function onCloseSplashRequested() {
            console.log("[QML] closeSplashRequested received. Closing redesigned splash.")
            statusTimer.stop()
            progressTimer.stop()
            progressFinishAnimation.stop()
            shimmerAnimation.stop()
            splashWindow.close()
        }
    }

    Timer {
        id: emergencyCloseTimer
        interval: 10000
        running: true
        repeat: false
        onTriggered: {
            console.log("[QML] Emergency splash timeout reached. Closing splash.")
            splashWindow.close()
        }
    }

    Timer {
        id: statusTimer
        interval: 350
        running: true
        repeat: true
        onTriggered: {
            dotPhase = (dotPhase + 1) % 4
        }
    }

    Timer {
        id: progressTimer
        interval: 70
        running: true
        repeat: true
        onTriggered: {
            if (visualProgress < 0.9) {
                visualProgress = Math.min(0.9, visualProgress + 0.012)
            }
        }
    }

    NumberAnimation {
        id: progressFinishAnimation
        target: splashWindow
        property: "visualProgress"
        to: 1.0
        duration: 220
        easing.type: Easing.OutCubic
    }

    Rectangle {
        anchors.fill: parent
        radius: 24
        color: "#0a0d12"
        border.width: 1
        border.color: "#203041"
        clip: true

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#0e141d" }
                GradientStop { position: 0.45; color: "#0a0d12" }
                GradientStop { position: 1.0; color: "#080a0f" }
            }
        }

        Rectangle {
            width: 420
            height: 420
            radius: 210
            color: "#101fcfef"
            opacity: 0.08
            x: -90
            y: -140
        }

        Rectangle {
            width: 380
            height: 380
            radius: 190
            color: "#10ffffff"
            opacity: 0.05
            anchors.right: parent.right
            anchors.rightMargin: -120
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -110
        }

        Column {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 18

            Rectangle {
                id: bannerCard
                width: parent.width
                height: 220
                radius: 18
                color: "#121821"
                border.width: 1
                border.color: "#223244"
                clip: true

                Image {
                    anchors.fill: parent
                    source: "qrc:/assets/banner.png"
                    fillMode: Image.PreserveAspectCrop
                    smooth: true
                    mipmap: true
                }

                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#00000000" }
                        GradientStop { position: 0.65; color: "#20000000" }
                        GradientStop { position: 1.0; color: "#b010141a" }
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    anchors.margins: 16
                    radius: 12
                    color: "#cc0d1219"
                    border.width: 1
                    border.color: "#2d7fa6"
                    width: brandColumn.width + 24
                    height: brandColumn.height + 18

                    Column {
                        id: brandColumn
                        anchors.centerIn: parent
                        spacing: 4

                        Text {
                            text: "BETTERANGLE PRO"
                            color: "white"
                            font.pixelSize: 22
                            font.bold: true
                            font.letterSpacing: 2.5
                        }

                        Text {
                            text: "Powered by WaveDropMaps"
                            color: "#93dfff"
                            font.pixelSize: 13
                            font.bold: true
                            font.letterSpacing: 1.2
                        }
                    }
                }
            }

            Item {
                width: parent.width
                height: 150

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    spacing: 10

                    Text {
                        text: "Launching BetterAngle"
                        color: "white"
                        font.pixelSize: 28
                        font.bold: true
                        font.letterSpacing: 1.5
                    }

                    Text {
                        text: "Preparing the angle HUD, profiles, and WaveDropMaps-powered startup experience."
                        color: "#9fb1c2"
                        font.pixelSize: 14
                        wrapMode: Text.WordWrap
                    }

                    Rectangle {
                        id: progressTrack
                        width: parent.width
                        height: 16
                        radius: 8
                        color: "#16212d"
                        border.width: 1
                        border.color: "#294156"
                        clip: true

                        Rectangle {
                            id: progressFill
                            width: Math.max(30, visualProgress * progressTrack.width)
                            height: parent.height
                            radius: 8
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: "#1bc6ff" }
                                GradientStop { position: 0.55; color: "#15f0c5" }
                                GradientStop { position: 1.0; color: "#94ff6d" }
                            }

                            Behavior on width {
                                NumberAnimation {
                                    duration: 160
                                    easing.type: Easing.OutCubic
                                }
                            }

                            Rectangle {
                                id: shimmer
                                width: 120
                                height: parent.height
                                radius: 8
                                opacity: 0.30
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#00ffffff" }
                                    GradientStop { position: 0.5; color: "#ccffffff" }
                                    GradientStop { position: 1.0; color: "#00ffffff" }
                                }
                            }

                            NumberAnimation {
                                id: shimmerAnimation
                                target: shimmer
                                property: "x"
                                from: -shimmer.width
                                to: Math.max(progressFill.width, shimmer.width)
                                duration: 1100
                                loops: Animation.Infinite
                                running: true
                            }
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 12

                        Text {
                            text: dotPhase === 0 ? "Loading" : dotPhase === 1 ? "Loading." : dotPhase === 2 ? "Loading.." : "Loading..."
                            color: "#d7e4ef"
                            font.pixelSize: 13
                            font.bold: true
                        }
                    }
                }
            }
        }
    }
}
