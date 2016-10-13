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
    plaintextlog.cpp \
    searchhighlighter.cpp \
    asyncserialport.cpp

HEADERS  += mainwindow.h \
    preferencesdialog.h \
    plaintextlog.h \
    searchhighlighter.h \
    logblockcustomdata.h \
    asyncserialport.h

FORMS    += mainwindow.ui \
    preferencesdialog.ui

DISTFILES +=
