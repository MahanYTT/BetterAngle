import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

Window {
    id: splashWindow
    width: 640
    height: 400
    x: Screen.width / 2 - width / 2
    y: Screen.height / 2 - height / 2
    visible: true
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool

    Connections {
        target: backend
        onCloseSplashRequested: {
            console.log("[QML] Close signal received. Terminating splash.")
            splashWindow.close()
        }
    }

    // "God Mode" Nuclear Fail-Safe (v4.27.10)
    Timer {
        interval: 8000
        running: true
        repeat: false
        onTriggered: {
            console.log("[QML] 8s Fail-Safe Triggered. Forcing splash close.")
            splashWindow.close()
        }
    }

    Rectangle {
        id: mainBg
        anchors.fill: parent
        color: "#050508"
        radius: 20
        border.color: "#1a1a25"
        border.width: 1
        clip: true

        // ── Background Glow ──────────────────────────────────────────
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#0d0d1a" }
                GradientStop { position: 1.0; color: "#050508" }
            }
        }

        // ── Wave Animation (The "Wave Thing") ────────────────────────
        Canvas {
            id: waveCanvas
            anchors.fill: parent
            opacity: 0.3
            property real phase: 0
            
            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);
                ctx.beginPath();
                ctx.strokeStyle = "#00ffa3";
                ctx.lineWidth = 2;
                
                var mid = height * 0.85;
                ctx.moveTo(0, mid);
                
                for (var x = 0; x <= width; x += 5) {
                    var y = mid + Math.sin(x * 0.01 + phase) * 20;
                    ctx.lineTo(x, y);
                }
                
                ctx.stroke();
            }

            NumberAnimation on phase {
                from: 0; to: Math.PI * 2; duration: 3000; loops: Animation.Infinite
            }

            onPhaseChanged: requestPaint()
        }

        // ── The Banner (Moved to Top v4.27.25) ───────────────────
        Item {
            width: parent.width - 100
            height: 80
            anchors.top: parent.top
            anchors.topMargin: 20
            anchors.horizontalCenter: parent.horizontalCenter
            clip: true

            Image {
                anchors.fill: parent
                source: "qrc:/assets/banner.png"
                fillMode: Image.PreserveAspectFit
                opacity: 0.8
            }
            
            Rectangle {
                width: parent.width; height: 1; color: "#00ffa3"; opacity: 0.1
                anchors.top: parent.top
                YAnimator on y { from: 0; to: 80; duration: 2000; loops: Animation.Infinite }
            }
        }

        // ── Content Wrapper (Centered Layout v4.27.25) ────────────────
        Item {
            anchors.fill: parent
            
            Column {
                anchors.centerIn: parent
                spacing: 20

                // New Circular Logo
                Item {
                    width: 100; height: 100
                    anchors.horizontalCenter: parent.horizontalCenter
                    
                    Rectangle {
                        anchors.fill: parent
                        radius: 50
                        color: "#11ffffff"
                        clip: true
                        
                        Image {
                            anchors.fill: parent
                            source: "qrc:/assets/logo.png"
                            fillMode: Image.PreserveAspectCrop
                        }

                        // Pulse Effect
                        Rectangle {
                            anchors.fill: parent
                            radius: 50
                            color: "transparent"
                            border.color: "#00ffa3"
                            border.width: 1.5
                            opacity: 0.6
                            
                            SequentialAnimation on scale {
                                loops: Animation.Infinite
                                NumberAnimation { from: 1.0; to: 1.2; duration: 1500; easing.type: Easing.OutCubic }
                                NumberAnimation { from: 1.2; to: 1.0; duration: 0 }
                            }
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                NumberAnimation { from: 0.6; to: 0.0; duration: 1500; easing.type: Easing.OutCubic }
                                NumberAnimation { from: 0.0; to: 0.6; duration: 0 }
                            }
                        }
                    }
                }

                // Centered Version Number
                Text {
                    text: "VERSION 4.27.25"
                    color: "#00ffa3"
                    font.pixelSize: 14
                    font.bold: true
                    font.letterSpacing: 4
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // Brand Main Text
                Text {
                    text: "BETTERANGLE PRO"
                    color: "white"
                    font.pixelSize: 32
                    font.bold: true
                    font.letterSpacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // The Quote
                Text {
                    text: "\"The best drops begin with the best wins\""
                    color: "#666"
                    font.pixelSize: 13
                    font.italic: true
                    font.letterSpacing: 1
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            // ── Hardlocked Progress Bar (3 Seconds) ────────────────
            Item {
                width: 340
                height: 4
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 40
                anchors.horizontalCenter: parent.horizontalCenter

                // Background
                Rectangle {
                    anchors.fill: parent
                    color: "#11ffffff"
                    radius: 2
                }

                // Actual Progress
                Rectangle {
                    id: progressBar
                    width: (backend.loadingProgress / 100.0) * 340
                    height: parent.height
                    color: "#00ffa3"
                    radius: 2
                    
                    Behavior on width {
                        NumberAnimation { duration: 400; easing.type: Easing.OutCubic }
                    }
                    
                    // Glow on the leading edge
                    Rectangle {
                        width: 8; height: 12
                        color: "#00ffa3"
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        opacity: 0.5
                        visible: progressBar.width > 0 && progressBar.width < 340
                        
                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            NumberAnimation { from: 0.5; to: 0.8; duration: 500 }
                            NumberAnimation { from: 0.8; to: 0.5; duration: 500 }
                        }
                    }
                }
                
                Text {
                    text: "LOADING ENGINE..."
                    color: "#444"
                    font.pixelSize: 9
                    font.bold: true
                    anchors.top: parent.bottom
                    anchors.topMargin: 8
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.letterSpacing: 2
                }
            }
        }
    }
}
