MOC_DIR = .build
OBJECTS_DIR = .build
RCC_DIR = .build
UI_DIR = .build

win32:!unix {
# Necessitas (or rather the cross compiler) can't handle these in the INCLUDEPATH, so I moved it out of wifisigmap.pri to here
# Necessitas still adds these to the INCLUDEPATH even though we specify win32:!unix
        INCLUDEPATH += "C:\\Program Files (x86)\\Microsoft Visual Studio 9.0\\VC\\include"
        INCLUDEPATH += "C:\\Program Files\\Microsoft SDKs\\Windows\\v7.0\\Include"
}

#QT += opengl

TEMPLATE = app
TARGET = wifisigmap
DEPENDPATH += .
INCLUDEPATH += .

include(wifisigmap.pri)

SOURCES += main.cpp