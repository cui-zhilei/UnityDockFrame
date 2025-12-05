#-------------------------------------------------
#
# Project created by QtCreator 2020-11-23T08:16:06
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = UnityDockFrame-Demo
TEMPLATE = app
DEFINES += DOCK_USE_DLL
CONFIG(debug, debug|release){
    DESTDIR = $$PWD/../Bin/Debug
} else {
    DESTDIR = $$PWD/../Bin/Release
}
LIBS += -L$$DESTDIR -lDock
QMAKE_RPATHDIR += $$DESTDIR

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    main.cpp \
    MainWindow.cpp \
    BlackWindow.cpp \
    CyanWindow.cpp \
    RedWindow.cpp \
    GreenWindow.cpp \
    BlueWindow.cpp

HEADERS += \
    MainWindow.h \
    MyDockContainer.h \
    BlackWindow.h \
    CyanWindow.h \
    RedWindow.h \
    GreenWindow.h \
    BlueWindow.h

INCLUDEPATH += ./../Dock

SUBDIRS += \
    UnityDockFrame-Demo.pro

DISTFILES += \
    UnityDockFrame-Demo.pro.user
