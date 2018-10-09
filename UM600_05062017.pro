#-------------------------------------------------
#
# Project created by QtCreator 2017-06-05T09:24:20
#
#-------------------------------------------------

QT       += core gui
QT      += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = UM600_05062017

TEMPLATE = app

RC_FILE = iconn.rc

SOURCES += main.cpp\
        mainwindow.cpp \
    port.cpp \
    comdial.cpp \
    consoll.cpp

HEADERS  += mainwindow.h \
    port.h \
    comdial.h \
    consoll.h

FORMS    += mainwindow.ui \
    comdial.ui

RESOURCES += \
    resource.qrc
