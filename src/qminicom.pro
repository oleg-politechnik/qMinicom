#-------------------------------------------------
#
# Project created by QtCreator 2016-09-13T15:30:05
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qminicom
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    preferencesdialog.cpp \
    plaintextlog.cpp

HEADERS  += mainwindow.h \
    preferencesdialog.h \
    plaintextlog.h

FORMS    += mainwindow.ui \
    preferencesdialog.ui

DISTFILES +=
