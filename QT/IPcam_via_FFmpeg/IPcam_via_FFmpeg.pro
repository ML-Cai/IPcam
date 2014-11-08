#-------------------------------------------------
#
# Project created by QtCreator 2014-11-06T19:08:10
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = IPcam_via_FFmpeg
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    glwidget.cpp \
    video_decoder.cpp

HEADERS  += mainwindow.h \
    glwidget.h \
    video_decoder.h \
    network_packet.h

FORMS    += mainwindow.ui

#-------------------------------------------------
# include path and library setting
#-------------------------------------------------
LIBS += -L/opt/FFmpeg/lib -lavutil -lavcodec -lavformat -lswscale -lswresample
INCLUDEPATH += /opt/FFmpeg/include
