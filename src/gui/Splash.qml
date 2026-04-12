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
    title: qsTr("BetterAngle Splash")
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    Component.onCompleted: {
        console.log("[QML] Splash window component completed successfully.")
    }

    onVisibleChanged: {
        console.log("[QML] Splash visibility changed:", visible)
    }

    Connections {
        target: backend
        function onCloseSplashRequested() {
            console.log("[QML] closeSplashRequested received. Stopping animations and closing splash window.")
            // Stop all animations to reduce CPU usage
            waveCanvas.phaseAnimation.stop()
            pulseScaleAnimation.stop()
            pulseOpacityAnimation.stop()
            glowOpacityAnimation.stop()
            lineYAnimator.stop()
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
            property alias phaseAnimation: wavePhaseAnimation
            
            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);
                ctx.beginPath();
                ctx.strokeStyle = "#00ffa3";
                ctx.lineWidth = 2;
                
                var mid = height * 0.85;
                ctx.moveTo(0, mid);
                
                // Reduce rendering frequency for better performance
                for (var x = 0; x <= width; x += 8) {
                    var y = mid + Math.sin(x * 0.01 + phase) * 20;
                    ctx.lineTo(x, y);
                }
                
                ctx.stroke();
            }

            NumberAnimation {
                id: wavePhaseAnimation
                target: waveCanvas
                property: "phase"
                from: 0; to: Math.PI * 2; duration: 4000; loops: Animation.Infinite
                running: true
            }

            onPhaseChanged: {
                // Throttle repaints to reduce CPU usage
                if (Math.floor(phase * 10) % 3 === 0) {
                    requestPaint()
                }
            }
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
                id: lineAnimator
                width: parent.width; height: 1; color: "#00ffa3"; opacity: 0.1
                anchors.top: parent.top
                property alias lineYAnimator: yAnim
                YAnimator {
                    id: yAnim
                    target: lineAnimator
                    from: 0; to: 80; duration: 3000; loops: Animation.Infinite
                    running: true
                }
            }
        }

        // ── Content Wrapper (Centered Layout v4.27.25) ────────────────
        Item {
            anchors.fill: parent
            
            Column {
                anchors.centerIn: parent
                spacing: 20

                // New Circular Logo (Hard-Locked 1:1 Aspect Ratio)
                Item {
                    width: 120; height: 120
                    anchors.horizontalCenter: parent.horizontalCenter
                    
                    Rectangle {
                        anchors.fill: parent
                        radius: 60
                        color: "#11ffffff"
                        clip: true
                        border.color: "#1a00ffa3"
                        border.width: 1
                        
                        Image {
                            anchors.centerIn: parent
                            width: 80; height: 80
                            source: "qrc:/assets/logo_transparent.png"
                            fillMode: Image.PreserveAspectFit
                            mipmap: true
                        }

                        // Pulse Effect (Hard-Centered)
                        Rectangle {
                            id: pulseRect
                            anchors.fill: parent
                            radius: 60
                            color: "transparent"
                            border.color: "#00ffa3"
                            border.width: 2
                            opacity: 0.6
                            property alias pulseScaleAnimation: scaleAnim
                            property alias pulseOpacityAnimation: opacityAnim
                            
                            SequentialAnimation on scale {
                                id: scaleAnim
                                loops: Animation.Infinite
                                running: true
                                NumberAnimation { from: 1.0; to: 1.10; duration: 2000; easing.type: Easing.OutCubic }
                                NumberAnimation { from: 1.10; to: 1.0; duration: 1000 }
                            }
                            SequentialAnimation on opacity {
                                id: opacityAnim
                                loops: Animation.Infinite
                                running: true
                                NumberAnimation { from: 0.6; to: 0.0; duration: 2000; easing.type: Easing.OutCubic }
                                NumberAnimation { from: 0.0; to: 0.6; duration: 1000 }
                            }
                        }
                    }
                }

        }
        
        // Centered Version Number (Exact Window Center v4.27.45)
        Text {
            text: "VERSION 4.27.45"
            color: "#00ffa3"
            font.pixelSize: 13
            font.bold: true
            font.letterSpacing: 4
            anchors.centerIn: parent
            opacity: 0.9
            z: 10 // Ensure it stays above animation layers
        }

        // ── Content Wrapper (Shifted down below top-banner v4.27.45) ────────
        Item {
            anchors.fill: parent
            anchors.topMargin: 100

                // Brand Main Text (Balanced)
                Text {
                    text: "BETTERANGLE PRO"
                    color: "white"
                    font.pixelSize: 30
                    font.bold: true
                    font.letterSpacing: 6
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // The Quote (Precision Aligned)
                Text {
                    text: "\"The best wins begin with the best drops\""
                    color: "#888"
                    font.pixelSize: 12
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
                        id: glowRect
                        width: 8; height: 12
                        color: "#00ffa3"
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        opacity: 0.5
                        visible: progressBar.width > 0 && progressBar.width < 340
                        property alias glowOpacityAnimation: glowAnim
                        
                        SequentialAnimation {
                            id: glowAnim
                            target: glowRect
                            property: "opacity"
                            loops: Animation.Infinite
                            running: true
                            NumberAnimation { from: 0.5; to: 0.7; duration: 800 }
                            NumberAnimation { from: 0.7; to: 0.5; duration: 800 }
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
