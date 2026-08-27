QT += core gui quick quickcontrols2 multimedia testlib
CONFIG += c++17 testcase
# Always a plain executable: a macOS .app bundle would put the binary three
# directories down and `./backend_tests` would not exist.
CONFIG -= app_bundle
TARGET = backend_tests
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += \
    ../src/backend.h \
    ../src/ffmpeg.h \
    ../src/filepicker.h \
    ../src/thumbprovider.h \
    ../src/thumbworker.h

SOURCES += \
    backend_tests.cpp \
    ../src/backend.cpp \
    ../src/ffmpeg.cpp \
    ../src/thumbprovider.cpp \
    ../src/thumbworker.cpp

# Mirrors omacut.pro: the tests link whichever file picker the platform builds.
linux {
    QT += dbus
    HEADERS += ../src/portalfilepicker.h
    SOURCES += ../src/portalfilepicker.cpp
} else {
    QT += widgets
    HEADERS += ../src/nativefilepicker.h
    SOURCES += ../src/nativefilepicker.cpp
}
