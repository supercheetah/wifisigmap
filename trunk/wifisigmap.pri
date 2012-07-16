VPATH += $$PWD
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD

QT += network

win32:!unix {
#        INCLUDEPATH += "C:\\Program Files (x86)\\Microsoft Visual Studio 9.0\\VC\\include"
#        INCLUDEPATH += "C:\\Program Files\\Microsoft SDKs\\Windows\\v7.0\\Include"
}

RESOURCES = resources.qrc

# Not used yet/now...
#include(3rdparty/wireless-tools/wireless-tools.pri)

# Input
HEADERS += \
	MapWindow.h \
	MapGraphicsScene.h \
	WifiDataCollector.h
SOURCES += \
	MapWindow.cpp \
	MapGraphicsScene.cpp \
	WifiDataCollector.cpp
