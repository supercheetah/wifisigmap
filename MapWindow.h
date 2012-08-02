#ifndef MapWindow_H
#define MapWindow_H

#include <QtGui>

class MapGraphicsView;
class MapGraphicsScene;

class MapWindow : 
#ifdef Q_OS_ANDROID
	public QWidget
#else
	public QMainWindow
#endif
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
	
	void toggleApMarkMode();
	
	void flagApModeCleared();
	void clearStatusMessage();

	void closeMainMenu();
	void showMainMenu();
	void toggleMainMenu();

	void autosave();

private:
	MapGraphicsView *m_gv;
	MapGraphicsScene *m_scene;
	QLabel *m_statusMsg;
	QPushButton *m_apButton;
	QAction *m_apAction;

	QTimer m_statusClearTimer;
	
	QToolButton *makeAndroidToolButton(QWidget *parent, QObject *obj, const char *slot, QString text, QString icon, int width=270);
	
	QWidget *m_mainMenuWidget;

	QTimer m_autosaveTimer;
};

#endif
