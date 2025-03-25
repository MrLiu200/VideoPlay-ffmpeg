QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    frmmain.cpp

HEADERS += \
    frmmain.h

FORMS += \
    frmmain.ui
CONFIG += console
INCLUDEPATH += $$PWD/ffmpeg
INCLUDEPATH += $$PWD/ffmpeg_v2
include     ($$PWD/ffmpeg/ffmpeg.pri)
include     ($$PWD/ffmpeg_v2/ffmpeg_v2.pri)

win32{
INCLUDEPATH += $$PWD/ffmpeg/ffmpeg7.0.2/windows/include
LIBS += -L$$PWD/ffmpeg/ffmpeg7.0.2/windows/lib/ -lavcodec\
        -L$$PWD/ffmpeg/ffmpeg7.0.2/windows/lib/ -lavdevice\
        -L$$PWD/ffmpeg/ffmpeg7.0.2/windows/lib/ -lavfilter\
        -L$$PWD/ffmpeg/ffmpeg7.0.2/windows/lib/ -lavformat\
        -L$$PWD/ffmpeg/ffmpeg7.0.2/windows/lib/ -lavutil\
        -L$$PWD/ffmpeg/ffmpeg7.0.2/windows/lib/ -lpostproc\
        -L$$PWD/ffmpeg/ffmpeg7.0.2/windows/lib/ -lswresample\
        -L$$PWD/ffmpeg/ffmpeg7.0.2/windows/lib/ -lswscale\
}

#win32{
#INCLUDEPATH += $$PWD/ffmpeg/ffmpeg7.0.2/windows/include
#LIBS += -L$$PWD/ffmpeg/ffmpeg7.0.2/bulid/lib/ -lavcodec\
#        -L$$PWD/ffmpeg/ffmpeg7.0.2/bulid/lib/ -lavdevice\
#        -L$$PWD/ffmpeg/ffmpeg7.0.2/bulid/lib/ -lavfilter\
#        -L$$PWD/ffmpeg/ffmpeg7.0.2/bulid/lib/ -lavformat\
#        -L$$PWD/ffmpeg/ffmpeg7.0.2/bulid/lib/ -lavutil\
#        -L$$PWD/ffmpeg/ffmpeg7.0.2/bulid/lib/ -lpostproc\
#        -L$$PWD/ffmpeg/ffmpeg7.0.2/bulid/lib/ -lswresample\
#        -L$$PWD/ffmpeg/ffmpeg7.0.2/bulid/lib/ -lswscale\
#}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

