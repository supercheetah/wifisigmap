#include "MapWindow.h"

#include <QApplication>

int main(int argc, char **argv)
{
	#if !defined(Q_OS_MAC) // raster on OSX == b0rken
		// use the Raster GraphicsSystem as default on 4.5+
		#if QT_VERSION >= 0x040500
		QApplication::setGraphicsSystem("raster");
		#endif
 	#endif
 	
	QApplication app(argc,argv);
	
	MapWindow win;
	win.show();
	win.move(562,0);
	win.resize(800,400);
	return app.exec();
}
