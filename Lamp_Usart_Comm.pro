#-------------------------------------------------
#
# Project created by QtCreator 2011-12-22T11:40:51
#
#-------------------------------------------------

QT       += core gui

TARGET = Lamp_Usart_Comm
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    win_qextserialport.cpp \
    qextserialbase.cpp

HEADERS  += mainwindow.h \
    win_qextserialport.h \
    qextserialbase.h

FORMS    += mainwindow.ui
