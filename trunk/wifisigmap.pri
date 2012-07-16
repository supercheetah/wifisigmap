VPATH += $$PWD
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD

QT += network

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
