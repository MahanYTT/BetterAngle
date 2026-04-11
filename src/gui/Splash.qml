import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

Window {
    id: splashWindow
    width: 600
    height: 380
    visible: true
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    Rectangle {
        id: mainBg
        anchors.fill: parent
        color: "#050508"
        radius: 16
        border.color: "#1a1a25"
        border.width: 1
        clip: true

        // Animated Topographic Waves
        Canvas {
            id: waveCanvas
            anchors.fill: parent
            opacity: 0.3
            
            property real phase: 0
            
            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);
                ctx.lineWidth = 1.5;
                
                var layers = [
                    { speed: 0.02, amp: 40, freq: 0.005, color: "#00ffa3", alpha: 0.4 },
                    { speed: 0.015, amp: 60, freq: 0.003, color: "#0080ff", alpha: 0.2 },
                    { speed: 0.01, amp: 30, freq: 0.007, color: "#ffffff", alpha: 0.1 }
                ];
                
                for (var i = 0; i < layers.length; i++) {
                    var l = layers[i];
                    ctx.strokeStyle = l.color;
                    ctx.globalAlpha = l.alpha;
                    ctx.beginPath();
                    
                    for (var x = 0; x <= width; x += 10) {
                        var y = height * 0.6 + Math.sin(x * l.freq + phase * l.speed) * l.amp 
                                + Math.cos(x * 0.004 + phase * 0.015) * 20;
                        if (x === 0) ctx.moveTo(x, y);
                        else ctx.lineTo(x, y);
                    }
                    ctx.stroke();
                }
            }
            
            Timer {
                interval: 16
                running: true
                repeat: true
                onTriggered: {
                    waveCanvas.phase += 1;
                    waveCanvas.requestPaint();
                }
            }
        }

        // Content Wrapper
        Item {
            anchors.fill: parent
            
            Column {
                anchors.centerIn: parent
                spacing: 30
                horizontalAlignment: Text.AlignHCenter

                // Logo with Glow
                Item {
                    width: 80; height: 80
                    anchors.horizontalCenter: parent.horizontalCenter
                    
                    Rectangle {
                        anchors.fill: parent
                        radius: 40
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#00ffa3" }
                            GradientStop { position: 1.0; color: "#0080ff" }
                        }
                        
                        Text {
                            anchors.centerIn: parent
                            text: "\x3E"
                            color: "white"
                            font.pixelSize: 36
                            font.bold: true
                        }

                        // Glow animation
                        Rectangle {
                            anchors.fill: parent
                            radius: 40
                            color: "transparent"
                            border.color: "#00ffa3"
                            border.width: 2
                            scale: 1.0
                            opacity: 1.0

                            SequentialAnimation on scale {
                                loops: Animation.Infinite
                                NumberAnimation { from: 1.0; to: 1.5; duration: 2500; easing.type: Easing.OutCubic }
                                NumberAnimation { from: 1.5; to: 1.0; duration: 0 }
                            }
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                NumberAnimation { from: 0.8; to: 0.0; duration: 2500; easing.type: Easing.OutCubic }
                                NumberAnimation { from: 0.0; to: 0.8; duration: 0 }
                            }
                        }
                    }
                }

                // Brand
                Column {
                    spacing: 8
                    horizontalAlignment: Text.AlignHCenter
                    Text {
                        text: "BETTERANGLE PRO"
                        color: "white"
                        font.pixelSize: 32
                        font.bold: true
                        font.letterSpacing: 4
                    }
                    Text {
                        text: "V E R S I O N  4 . 2 2 . 2"
                        color: "#666"
                        font.pixelSize: 10
                        font.bold: true
                        font.letterSpacing: 2
                    }
                }
            }

            // Provided Banner Image
            Rectangle {
                width: parent.width
                height: 100
                color: "#1a000000"
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 40
                clip: true
                
                Image {
                    anchors.fill: parent
                    source: "qrc:/assets/banner.png"
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.9
                }

                // Top/Bottom border accents
                Rectangle {
                    width: parent.width; height: 1; color: "#00ffa3"
                    anchors.top: parent.top; opacity: 0.3
                }
                Rectangle {
                    width: parent.width; height: 1; color: "#00ffa3"
                    anchors.bottom: parent.bottom; opacity: 0.3
                }
                
                // Animated scan line
                Rectangle {
                    width: 120; height: parent.height
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 0.5; color: "#3300ffa3" }
                        GradientStop { position: 1.0; color: "transparent" }
                    }
                    XAnimator on x {
                        from: -120; to: splashWindow.width; duration: 2500; loops: Animation.Infinite
                    }
                }
            }

            // Minimal Progress Bar
            Rectangle {
                width: 300
                height: 2
                color: "#11ffffff"
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                anchors.horizontalCenter: parent.horizontalCenter

                Rectangle {
                    id: progressBar
                    width: 0
                    height: parent.height
                    color: "#00ffa3"
                    
                    NumberAnimation on width {
                        from: 0; to: 300; duration: 2200; easing.type: Easing.InOutQuad
                    }
                }
            }
        }
    }

    Timer {
        interval: 2500
        running: true
        repeat: false
        onTriggered: {
            splashWindow.close()
            backend.requestShowControlPanel()
        }
    }
}

