QT += core gui widgets
CONFIG += c++17

TARGET = FileModifier
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/fileprocessor.cpp

HEADERS += \
    include/mainwindow.h \
    include/fileprocessor.h

FORMS += \
    ui/mainwindow.ui

INCLUDEPATH += include

# Настройки релиза
CONFIG(release, debug|release) {
    DESTDIR = release
    OBJECTS_DIR = release/obj
    MOC_DIR = release/moc
    UI_DIR = release/ui
}