#ifndef MainWindow_H
#define MainWindow_H

#include <QtGui>

class MapWindow;

class MainWindow : public QWidget
{
	Q_OBJECT
public:
	MainWindow();
	~MainWindow();

protected:
	void resizeEvent(QResizeEvent*);

private:	
	MapWindow *m_map;

};

#endif
