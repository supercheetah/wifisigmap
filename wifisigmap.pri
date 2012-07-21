VPATH += $$PWD
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD

QT += network

RESOURCES = resources.qrc

# Not used yet/now...
#include(3rdparty/wireless-tools/wireless-tools.pri)

opencv: {
	DEFINES += OPENCV_ENABLED
	LIBS += -L/usr/local/lib -lcv -lcxcore -L/opt/OpenCV-2.1.0/lib
	INCLUDEPATH += /opt/OpenCV-2.1.0/include
	
	HEADERS += \
		KalmanFilter.h
	SOURCES += \
		KalmanFilter.cpp
}

# Input
FORMS += OptionsDialog.ui
HEADERS += \
	MapWindow.h \
	MapGraphicsScene.h \
	WifiDataCollector.h \
	OptionsDialog.h \
	SigMapRenderer.h \
	LongPressSpinner.h \
	MapGraphicsView.h
SOURCES += \
	MapWindow.cpp \
	MapGraphicsScene.cpp \
	WifiDataCollector.cpp \
	OptionsDialog.cpp \
	SigMapRenderer.cpp \
	LongPressSpinner.cpp \
	MapGraphicsView.cpp
