MOC_DIR = .build
OBJECTS_DIR = .build
RCC_DIR = .build
UI_DIR = .build


QT += opengl

TEMPLATE = app
TARGET = wifisigmap
DEPENDPATH += .
INCLUDEPATH += .

include(wifisigmap.pri)

SOURCES += main.cpp