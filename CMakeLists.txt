cmake_minimum_required(VERSION 3.16)
project(BetterAngle VERSION 4.27.113 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Qml REQUIRED)

# Read the version to pass definitions to C++ code
file(READ "VERSION" VER_STR)
string(STRIP "${VER_STR}" VER_STR)
string(REPLACE "." ";" VER_LIST "${VER_STR}")
list(GET VER_LIST 0 V_MAJ)
list(GET VER_LIST 1 V_MIN)
list(GET VER_LIST 2 V_PAT)

add_definitions(-DUNICODE -D_UNICODE
               -DV_MAJ=${V_MAJ} -DV_MIN=${V_MIN} -DV_PAT=${V_PAT})

set(SRC_FILES
    src/main_app/BetterAngle.cpp
    src/shared/ControlPanel.cpp
    include/shared/BetterAngleBackend.h
    src/shared/BetterAngleBackend.cpp
    include/shared/BetterAngleBackend.h
    src/shared/Detector.cpp
    src/shared/Input.cpp
    src/shared/Logic.cpp
    src/shared/Overlay.cpp
    src/shared/Profile.cpp
    src/shared/State.cpp
    src/shared/EnhancedLogging.cpp
    src/shared/Tray.cpp
    src/shared/Updater.cpp
    qml.qrc
    assets/resource.rc
)

add_executable(BetterAngle WIN32 ${SRC_FILES})

target_include_directories(BetterAngle PRIVATE include src)
target_link_libraries(BetterAngle PRIVATE 
    Qt6::Core Qt6::Gui Qt6::Qml
    user32 gdi32 gdiplus dwmapi winhttp shell32 d2d1 dwrite windowscodecs comdlg32 advapi32
)
