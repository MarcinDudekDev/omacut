QT += core gui qml quick quickcontrols2 multimedia

CONFIG += c++17 release
TARGET = omacut
TEMPLATE = app
VERSION = 0.4.0

HEADERS += \
    src/filepicker.h \
    src/ffmpeg.h \
    src/thumbworker.h \
    src/thumbprovider.h \
    src/backend.h

SOURCES += \
    src/main.cpp \
    src/ffmpeg.cpp \
    src/thumbworker.cpp \
    src/thumbprovider.cpp \
    src/backend.cpp

# The file picker is the one genuinely platform-bound piece: Linux talks to
# xdg-desktop-portal over D-Bus, everyone else uses the system's own panels
# through QtWidgets.
linux {
    QT += dbus
    HEADERS += src/portalfilepicker.h
    SOURCES += src/portalfilepicker.cpp
} else {
    QT += widgets
    HEADERS += src/nativefilepicker.h
    SOURCES += src/nativefilepicker.cpp
}

macx {
    QMAKE_TARGET_BUNDLE_PREFIX = io.omacom
    QMAKE_BUNDLE = omacut
    QMAKE_INFO_PLIST = macos/Info.plist
    ICON = macos/omacut.icns
}

RESOURCES += src/resources.qrc
