#-------------------------------------------------
#
# Project created by QtCreator 2018-12-21T16:50:49
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Mqtt
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    mqttclient.cpp \
    commonwin.cpp

HEADERS += \
        mainwindow.h \
    mqtt/include/qmqtt/qmqttDepends \
    mqttclient.h \
    publisher.h \
    subscriber.h

FORMS += \
        mainwindow.ui

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/ -lqmqtt
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/ -lqmqttd
else:unix: LIBS += -L$$PWD/lib/ -lqmqtt

INCLUDEPATH += $$PWD/lib
DEPENDPATH += $$PWD/lib

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/ -lqmqtt
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/ -lqmqttd
else:unix: LIBS += -L$$PWD/lib/ -lqmqtt

INCLUDEPATH += $$PWD/lib
DEPENDPATH += $$PWD/lib
