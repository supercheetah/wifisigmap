#ifndef MapWindow_H
#define MapWindow_H

#include <QtGui>

class MapGraphicsView;
class MapGraphicsScene;

class MapWindow : public QWidget
{
	Q_OBJECT
public:
	MapWindow(QWidget *parent=0);

protected:
	void setupUi();
	
	friend class MapGraphicsScene; // friend for access to the following (and flagApModeCleared())
	void setStatusMessage(const QString&, int timeout=-1);
	
	MapGraphicsView *gv() { return m_gv; }
	
	bool eventFilter(QObject *obj, QEvent *event);

protected slots:
	void saveSlot();
	void loadSlot();
	void chooseBgSlot();
	void clearSlot();
	void prefsSlot();
	
	void flagApModeCleared();
	void clearStatusMessage();

private:
	MapGraphicsView *m_gv;
	MapGraphicsScene *m_scene;
	QLabel *m_statusMsg;
	QPushButton *m_apButton;

	QTimer m_statusClearTimer;
};

#endif
