MOC_DIR = .build
OBJECTS_DIR = .build
RCC_DIR = .build
UI_DIR = .build

TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

RESOURCES = resources.qrc

# Input
HEADERS += \
	MapWindow.h \
	WifiDataCollector.h
SOURCES += \
	main.cpp \
	MapWindow.cpp \
	WifiDataCollector.cpp
