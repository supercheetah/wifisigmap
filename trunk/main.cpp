#include "MapWindow.h"

#include <QApplication>

int main(int argc, char **argv)
{
	QApplication app(argc,argv);
	
	MapWindow win;
	win.show();
	return app.exec();
}
