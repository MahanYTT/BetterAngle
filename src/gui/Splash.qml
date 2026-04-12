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
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.SplashScreen

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

        // ── Content Wrapper ─────────────────────────────────────────
        Item {
            anchors.fill: parent
            
            Column {
                anchors.centerIn: parent
                spacing: 24

                // Rotating Brand Icon
                Item {
                    width: 70; height: 70
                    anchors.horizontalCenter: parent.horizontalCenter
                    
                    Rectangle {
                        anchors.fill: parent
                        radius: 35
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#00ffa3" }
                            GradientStop { position: 1.0; color: "#0080ff" }
                        }
                        
                        Text {
                            anchors.centerIn: parent
                            text: "\x3E" // ">" symbol
                            color: "white"
                            font.pixelSize: 32
                            font.bold: true
                        }

                        // Pulse Effect
                        Rectangle {
                            anchors.fill: parent
                            radius: 35
                            color: "transparent"
                            border.color: "#00ffa3"
                            border.width: 1.5
                            opacity: 0.6
                            
                            SequentialAnimation on scale {
                                loops: Animation.Infinite
                                NumberAnimation { from: 1.0; to: 1.4; duration: 1500; easing.type: Easing.OutCubic }
                                NumberAnimation { from: 1.4; to: 1.0; duration: 0 }
                            }
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                NumberAnimation { from: 0.6; to: 0.0; duration: 1500; easing.type: Easing.OutCubic }
                                NumberAnimation { from: 0.0; to: 0.6; duration: 0 }
                            }
                        }
                    }
                }

                // Brand Main Text
                Column {
                    spacing: 4
                    Text {
                        text: "BETTERANGLE PRO"
                        color: "white"
                        font.pixelSize: 32
                        font.bold: true
                        font.letterSpacing: 6
                    }
                    Text {
                        text: "VERSION 4.26.3"
                        color: "#00ffa3"
                        font.pixelSize: 10
                        font.bold: true
                        font.letterSpacing: 3
                        opacity: 0.8
                    }
                }

                // The Quote (Centered)
                Text {
                    text: "\"The best drops begin with the best wins\""
                    color: "#888"
                    font.pixelSize: 14
                    font.italic: true
                    font.letterSpacing: 1
                    topPadding: 10
                }
            }

            // ── The Banner (Loading Screen Style) ───────────────────
            Item {
                width: parent.width - 100
                height: 80
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 80
                anchors.horizontalCenter: parent.horizontalCenter
                clip: true

                Image {
                    anchors.fill: parent
                    source: "qrc:/assets/banner.png"
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.6
                }
                
                // Subtle scanline overlay
                Rectangle {
                    width: parent.width; height: 1; color: "#00ffa3"; opacity: 0.1
                    anchors.top: parent.top
                    YAnimator on y {
                        from: 0; to: 80; duration: 2000; loops: Animation.Infinite
                    }
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
                    width: 0
                    height: parent.height
                    color: "#00ffa3"
                    radius: 2
                    
                    // The 3-second hard lock animation
                    NumberAnimation on width {
                        from: 0; to: 340; duration: 3000; easing.type: Easing.InOutSine
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

    // ── 3-Second Hard Lock Timer ───────────────────────────────────
    Timer {
        interval: 3000 // 3000ms = 3 Seconds
        running: true
        repeat: false
        onTriggered: {
            splashWindow.close()
        }
    }
}
