VPATH += $$PWD
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD

QT += network

### Can't just include the gfxengine.pri because it uses OpenGL and Android devices cant use openGL yet.
##include(../gfxengine/gfxengine.pri)
##DEFINES += NO_OPENGL

##INCLUDEPATH += $$PWD/../gfxengine
##DEPENDPATH += $$PWD/../gfxengine

##QT += multimedia

### Used for the HTTP interface to the host
##include(../3rdparty/qjson/qjson.pri)

RESOURCES = resources.qrc

# Input
HEADERS += \
	MapWindow.h \
	WifiDataCollector.h
SOURCES += \
	MapWindow.cpp \
	WifiDataCollector.cpp
