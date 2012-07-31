#include "MapWindow.h"

#include "MapGraphicsView.h"
#include "MapGraphicsScene.h"
#include "OptionsDialog.h"
#include "3rdparty/FlowLayout.h"

#include <QTcpSocket>
#include <QApplication>

#define CUSTOM_MSG_HANDLER

#if defined(CUSTOM_MSG_HANDLER)

	#ifdef Q_OS_WIN
		extern Q_CORE_EXPORT void qWinMsgHandler(QtMsgType t, const char* str);
	#endif

	static QTcpSocket *qt_debugSocket = 0;
	static QtMsgHandler qt_origMsgHandler = 0;

	void myMessageOutput(QtMsgType type, const char *msg)
	{
		#if defined(Q_OS_WIN)
		if (!qt_origMsgHandler)
			qt_origMsgHandler = qWinMsgHandler;
		#endif

		switch (type)
		{
			case QtDebugMsg:
				if(qt_debugSocket->state() == QAbstractSocket::ConnectedState)
					qt_debugSocket->write(QByteArray((QString(msg) + "\n").toAscii()));
				//AppSettings::sendCheckin("/core/debug",QString(msg));
				if(qt_origMsgHandler)
					qt_origMsgHandler(type,msg);
				else
					fprintf(stderr, "%s\n", msg);
				break;
			case QtWarningMsg:
				if(qt_debugSocket->state() == QAbstractSocket::ConnectedState)
					qt_debugSocket->write(QByteArray((QString(msg) + "\n").toAscii()));
				//AppSettings::sendCheckin("/core/warn",QString(msg));
				if(qt_origMsgHandler)
					qt_origMsgHandler(QtDebugMsg,msg);
				else
					fprintf(stderr, "Warning: %s\n", msg);
				break;
			case QtCriticalMsg:
				if(qt_debugSocket->state() == QAbstractSocket::ConnectedState)
					qt_debugSocket->write(QByteArray((QString(msg) + "\n").toAscii()));
	// 			if(qt_origMsgHandler)
	// 				qt_origMsgHandler(type,msg);
	// 			else
	// 				fprintf(stderr, "Critical: %s\n", msg);
	// 			break;
			case QtFatalMsg:
				if(qt_debugSocket->state() == QAbstractSocket::ConnectedState)
					qt_debugSocket->write(QByteArray((QString(msg) + "\n").toAscii()));
				//AppSettings::sendCheckin("/core/fatal",QString(msg));
				if(qt_origMsgHandler)
				{
					qt_origMsgHandler(QtDebugMsg,msg);
					//qt_origMsgHandler(type,msg);
				}
				else
				{

					fprintf(stderr, "Fatal: %s\n", msg);
				}
				//QMessageBox::critical(0,"Fatal Error",msg);
				//qt_origMsgHandler(QtDebugMsg,msg);
				/*
				if(strstr(msg,"out of memory, returning null image") != NULL)
				{
					QPixmapCache::clear();
					qt_origMsgHandler(QtDebugMsg, "Attempted to clear QPixmapCache, continuing");
					return;
				}
				*/
				abort();
		}
	}
#endif // CUSTOM_MSG_HANDLER


MapWindow::MapWindow(QWidget *parent)
#ifdef Q_OS_ANDROID
	: QWidget(parent)
#else
	: QMainWindow(parent)
#endif
{
	#ifdef CUSTOM_MSG_HANDLER
		qt_debugSocket = new QTcpSocket(this);
		//qt_debugSocket->connectToHost("192.168.2.104", 3729);
		qt_debugSocket->connectToHost("10.10.9.90", 3729);
		//qt_debugSocket->connectToHost("10.1.5.181", 3729);
		qt_origMsgHandler = qInstallMsgHandler(myMessageOutput);
	#endif
	
	#ifdef Q_OS_ANDROID
	// 96px was a bit big, especially on Kindle Fire
	// TODO need a way to determine physical screen size (mm)
	// TODO use 'int QPaintDevice::physicalDpiX ()' (and DpiY) to adjust properly?)
	// TODO maybe use int QPaintDevice::heightMM () / width() and widthMM() / height() 
	// NOTE http://stackoverflow.com/a/2019766 cites Target Size Study for One-Handed Thumb Use on Small Touchscreen Devices (Parhi, Karlson, & Bederson 2006). Other sources agree on this "close-to-0.4-inch-rule" (e.g. Designing Gestural Interfaces (Saffer 2008, p. 42))
	//	- Which gives 9.2mm-9.6mm or approx 0.4in
	QSize strut(physicalDpiX() * 0.20,physicalDpiY() * 0.20);
	//qDebug() << "MapWindow: Setting global strut: "<<strut<<", based on:"<<physicalDpiX()<<" x "<<physicalDpiY()<<" dpi";
	QApplication::setGlobalStrut(strut);
	#endif
	
	setWindowTitle("WiFi Signal Mapper");
	setWindowIcon(QPixmap(":/data/images/icon.png"));
	setupUi();
	
	QString lastFile = QSettings("wifisigmap").value("last-map-file","").toString();
	if(!lastFile.isEmpty())
		m_scene->loadResults(lastFile);
	
	/// NOTE just for testing
	//m_scene->loadResults("wmz/phc-firsttest.wmz");
	//m_scene->loadResults("wmz/test.wmz");
	//m_scene->loadResults("wmz/phcfirstrun-track2.wmz");
	//m_scene->loadResults("wmz/phcfirstrun.wmz");
	//m_scene->loadResults("wmz/foobar.wmz");
	//m_scene->loadResults("wmz/test-track3.wmz");
	//m_scene->loadResults("wmz/test.wmz");
	//m_scene->loadResults("wmz/pci4000-livedata2.wmz");
	//m_scene->loadResults("wmz/pci-track-test.wmz");

	#ifdef Q_OS_ANDROID
	QFile styleSheet(":/data/android/styles.qss");
	if(!styleSheet.open(QIODevice::ReadOnly))
	{
		qWarning("Unable to read :/data/android/styles.qss");
	}
	else
	{
		int fontSize = (int)((((double)physicalDpiY()) / 276.)  * 24);
		QString qss = styleSheet.readAll();
		qss.replace("font-size: 22px", QString("font-size: %1px").arg(fontSize));
		qApp->setStyleSheet(qss);
	}

	// So we can figure out which key is the meny key...
	if(parent)
		parent->installEventFilter(this);
	#endif
	
	// Just here for debugging
	//showMainMenu();
}

bool MapWindow::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
		//qDebug() << "Got key press" << keyEvent->key();
		if(keyEvent->key() == 16777301)
			showMainMenu();
	}
	
	// pass the event on to the parent class
	return QWidget::eventFilter(obj, event);
}

#define makeButton2(object,layout,title,slot) \
	{ QPushButton *btn = new QPushButton(title); \
		connect(btn, SIGNAL(clicked()), object, slot); \
		layout->addWidget(btn); \
	}
	
#define makeButton(layout,title,slot) \
	makeButton2(this,layout,title,slot)
	
#define makeAction(title,slot,shortcut) \
	makeAction2(title,this,slot,shortcut);
	
#define makeAction2(title,object,slot,shortcut) \
	{ QAction *act = new QAction(tr(title), this); \
		act->setShortcuts(shortcut); \
		connect(act, SIGNAL(triggered()), object, slot); \
		menu->addAction(act); \
	}
	
void MapWindow::setupUi()
{
	m_gv = new MapGraphicsView();
	m_scene = new MapGraphicsScene(this);
	m_gv->setMapScene(m_scene);
	
	m_apAction = 0;
	m_mainMenuWidget = 0;
	
	#ifndef Q_OS_ANDROID
	QMenu *menu;
	 
	menu = menuBar()->addMenu(tr("&File"));
	
	makeAction("&New",		SLOT(clearSlot()),	QKeySequence::New);
	makeAction("&Open...",		SLOT(loadSlot()),	QKeySequence::Open);
	makeAction("&Save As...",	SLOT(saveSlot()),	QKeySequence::SaveAs);
	
	menu->addSeparator();
	makeAction2("E&xit",	qApp,	SLOT(quit()),		QKeySequence::Quit);
	
	
	menu = menuBar()->addMenu(tr("&Edit"));
	makeAction("Choose &Background...", SLOT(chooseBgSlot()), QKeySequence::UnknownKey); // 2nd arg to tr() is translator comment
	
	m_apAction = new QAction(tr("&Mark AP Location"), this);
	m_apAction->setCheckable(true);
	connect(m_apAction, SIGNAL(toggled(bool)), m_scene, SLOT(setMarkApMode(bool)));
	menu->addAction(m_apAction);
	
	menu->addSeparator();
	makeAction("&Options...", SLOT(prefsSlot()), QKeySequence::Preferences);
	#endif
	
	#ifdef Q_OS_ANDROID
	QVBoxLayout *vbox = new QVBoxLayout(this);
	
	#else
	QWidget *centerWidget = new QWidget(this);
	setCentralWidget(centerWidget);
	
	QVBoxLayout *vbox = new QVBoxLayout(centerWidget);
	vbox->setContentsMargins(0,0,0,0);
	#endif
	
// 	QHBoxLayout *hbox;
// 	hbox = new QHBoxLayout();
// 	
// 	makeButton(hbox, "New",  SLOT(clearSlot()));
// 	makeButton(hbox, "Load", SLOT(loadSlot()));
// 	makeButton(hbox, "Save", SLOT(saveSlot()));
	
// 	#ifndef Q_OS_ANDROID
// 	// Disable on android because until I rewrite the file browser dialog,
// 	// the file browser is pretty much useless
// 	makeButton(hbox, "Background...", SLOT(chooseBgSlot()));
// 	#endif
	
// 	makeButton2(m_gv, hbox, "+", SLOT(zoomIn()));
// 	makeButton2(m_gv, hbox, "-", SLOT(zoomOut()));
// 	
// 	vbox->addLayout(hbox);
	
	vbox->addWidget(m_gv);
	
	
// 	hbox = new QHBoxLayout();
// 	m_apButton = new QPushButton("Mark AP");
// 	m_apButton->setCheckable(true);
// 	m_apButton->setChecked(false);
// 	connect(m_apButton, SIGNAL(toggled(bool)), m_scene, SLOT(setMarkApMode(bool)));
// 	hbox->addWidget(m_apButton);

	
// 	m_statusMsg = new QLabel("");
// 	hbox->addWidget(m_statusMsg);
// 	
// 	hbox->addStretch(1);
// 	
// 	#ifdef Q_OS_ANDROID
// 	QString size = "64x64";
// 	#else
// 	QString size = "32x32";
// 	#endif
// 	
// 	QPushButton *prefs = new QPushButton(QPixmap(tr(":/data/images/%1/stock-preferences.png").arg(size)), "");
// 	connect(prefs, SIGNAL(clicked()), this, SLOT(prefsSlot()));
// 	hbox->addWidget(prefs);
// 	
	connect(&m_statusClearTimer, SIGNAL(timeout()), this, SLOT(clearStatusMessage()));
	m_statusClearTimer.setSingleShot(true);
	
//	vbox->addLayout(hbox);

	m_gv->setFocus();
}

void MapWindow::prefsSlot()
{
	/*
	QStringList items;
	items << "Radial Lines";
	items << "Circle";
	//items << "Triangles";
	
	bool ok;
	QString item = QInputDialog::getItem(0, tr("Render Style"),
						tr("Style:"), items, (int)m_scene->renderMode(), false, &ok);
	if (ok && !item.isEmpty())
	{
		m_scene->setRenderMode((MapGraphicsScene::RenderMode)items.indexOf(item));
	}
	*/

	OptionsDialog d(m_scene);
	d.show();
	//d.adjustSize();
	AndroidDialogHelper(this, d);
	d.exec();
}

void MapWindow::setStatusMessage(const QString& msg, int timeout)
{
	#ifndef Q_OS_ANDROID
	statusBar()->showMessage(msg, timeout);
	#endif
	
// 	#ifdef Q_OS_ANDROID
// 	m_statusMsg->setText("<font size='-2'>"+msg+"</b>");
// 	#else
// 	m_statusMsg->setText("<b>"+msg+"</b>");
// 	#endif
//
	m_gv->setStatusMessage(msg);
	//qDebug() << "MapWindow::setStatusMessage("<<msg<<","<<timeout<<")";

	if(timeout > 0)
	{
		if(m_statusClearTimer.isActive())
			m_statusClearTimer.stop();

		m_statusClearTimer.setInterval(timeout);
		m_statusClearTimer.start();
	}
	
	// Allow the UI to update in case the caller's next action is going to block the UI thread for any amount of time
	QApplication::processEvents();
}

void MapWindow::clearStatusMessage()
{
	#ifndef Q_OS_ANDROID
	statusBar()->clearMessage();
	//#else
	//setStatusMessage("");
	#endif

	setStatusMessage("");
}

void MapWindow::flagApModeCleared()
{
	clearStatusMessage();
	
	if(m_apAction)
		m_apAction->setChecked(false);
}

void MapWindow::chooseBgSlot()
{
	QString curFile = m_scene->currentBgFilename();
	if(curFile.trimmed().isEmpty())
		curFile = QSettings("wifisigmap").value("last-bg-file","").toString();

	QString fileName = QFileDialog::getOpenFileName(this, tr("Choose Background"), curFile, tr("Image Files (*.png *.jpg *.bmp *.svg *.xpm *.gif);;Any File (*.*)"));
	if(fileName != "")
	{
		QSettings("wifisigmap").setValue("last-bg-file",fileName);
		m_scene->setBgFile(fileName);

		QInputDialog d(this);
		d.setInputMode(QInputDialog::DoubleInput);
		d.setLabelText("Pixels per foot:");
		d.setDoubleMinimum(1.0);
		d.setDoubleMaximum(1000000);
		d.setDoubleDecimals(2);
		d.setDoubleValue(42.);
		d.show();
		
		AndroidDialogHelper(this, d);

		if(d.exec() == QDialog::Accepted)
			m_scene->setPixelsPerFoot(d.doubleValue());
	}
}

void MapWindow::loadSlot()
{
	/// TODO just for debugging till the filedialog works better on android
	#ifdef Q_OS_ANDROID2
	setStatusMessage(tr("Loading..."));
	m_scene->loadResults("/tmp/test.wmz");
	setStatusMessage(tr("<font color='green'>Loaded /tmp/test.wmz</font>"), 3000);
	return;
	#endif
	
	
	QString curFile = m_scene->currentMapFilename();
	if(curFile.trimmed().isEmpty())
		curFile = QSettings("wifisigmap").value("last-map-file","").toString();

	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Results"), curFile, tr("Wifi Signal Map (*.wmz);;Any File (*.*)"));
	if(fileName != "")
	{
		setStatusMessage(tr("Loading..."));
		QSettings("wifisigmap").setValue("last-map-file",fileName);
		m_scene->loadResults(fileName);
// 		if(openFile(fileName))
// 		{
// 			return;
// 		}
// 		else
// 		{
// 			QMessageBox::critical(this,tr("File Does Not Exist"),tr("Sorry, but the file you chose does not exist. Please try again."));
// 		}
		setStatusMessage(tr("<font color='green'>Loaded %1</font>").arg(fileName), 3000);
	}
}

void MapWindow::saveSlot()
{
	/// TODO just for debugging till the filedialog works better on android
	#ifdef Q_OS_ANDROID2
	setStatusMessage(tr("Saving..."));
	m_scene->saveResults("/tmp/test.wmz");
	setStatusMessage(tr("<font color='green'>Saved /tmp/test.wmz</font>"), 3000);
	return;
	#endif
	
	QString curFile = m_scene->currentMapFilename();
	if(curFile.trimmed().isEmpty())
		curFile = QSettings("wifisigmap").value("last-map-file","").toString();

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Results"), curFile, tr("Wifi Signal Map (*.wmz);;Any File (*.*)"));
	if(fileName != "")
	{
		setStatusMessage(tr("Saving..."));
		
		QFileInfo info(fileName);
		//if(info.suffix().isEmpty())
			//fileName += ".dviz";
		QSettings("wifisigmap").setValue("last-map-file",fileName);
		m_scene->saveResults(fileName);
// 		return true;
		setStatusMessage(tr("<font color='green'>Saved %1</font>").arg(fileName), 3000);
	}

}

void MapWindow::clearSlot()
{
	QSettings("wifisigmap").setValue("last-map-file","");
	m_scene->clear();
}

#define AndroidButtonDarkColor "rgb(180,180,180)"
QToolButton *MapWindow::makeAndroidToolButton(QWidget *parent, QObject *obj, const char *slot, QString text, QString icon, int width)
{
	QToolButton* btn = new QToolButton(parent);
	btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	btn->setIcon(QPixmap(icon));
	btn->setText(text);
	btn->setStyleSheet(QString(
			"QToolButton { "
				"width:         %1; "
 				"height:        100; "
				"padding:       0; "
 				"margin:        0; "
 				"padding-top:   10px; "
				"outline:       none; "
				"font-size:     20px; "
				"border-bottom: 1px solid " AndroidButtonDarkColor "; "
				"border-right:  1px solid " AndroidButtonDarkColor "; "
				"border-top:    1px solid white; "
				"border-left:   1px solid white; "
			"} "
			"QToolButton:focus:pressed, QToolButton:pressed {"
				"border-top:    1px solid " AndroidButtonDarkColor "; "
				"border-left:   1px solid " AndroidButtonDarkColor "; "
				"border-bottom: 1px solid white; "
				"border-right:  1px solid white; "
				"padding-top:   12px; "
				"padding-left:  2px; "
				"background:    #e7e7e7; "
				
			"}").arg(width));
	btn->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	btn->setIconSize(QSize(48, 48));
	btn->setMinimumSize(QSize(width,100));
	btn->setMaximumSize(QSize(width,100));
	
	connect(btn, SIGNAL(clicked()), obj, slot);
	connect(btn, SIGNAL(clicked()), m_mainMenuWidget, SLOT(hide()));
	connect(btn, SIGNAL(clicked()), m_mainMenuWidget, SLOT(deleteLater()));
	
	return btn;
}

void MapWindow::showMainMenu()
{
	QWidget *base = new QWidget();
	m_mainMenuWidget = base;
	
	//base->setAttribute(Qt::WA_TranslucentBackground, true);
	
	base->setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip);
	
	FlowLayout *layout = new FlowLayout(base, 0, 0, 0);
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(0);
	
	//base->resize(544,960);
	QDesktopWidget *desktop = qApp->desktop();
	base->resize(desktop->size());
	
	int numButtons = 6; // the number of buttons we plan to use (below)
	
	int btnWidth = base->width() / numButtons;
	if(btnWidth < 200) // arbitrary minimum size
		btnWidth = 200;
	
	int numPerRow = (int)(base->width() / btnWidth);
	btnWidth = base->width() / numPerRow - 1; // allow for border
	
	int numRows = numButtons / numPerRow;
	
	//qDebug() << "Using numPerRow:"<<numPerRow<<", btnWidth:"<<btnWidth<<", numRows: "<<numRows;
	
	base->resize(base->width(), numRows * 100); // 100 is height of button
	
	base->move(0, desktop->height() - base->height());
	
// 	QWidget *shadow = new QWidget(base);
// 	shadow->setMinimumSize(QSize(base->width(),20));
// 	shadow->setMaximumSize(QSize(base->width(),20));
// 	shadow->setStyleSheet("QWidget { background: url('data/images/simple-shadow.png'); height: 20px; }");
// 	
// 	layout->addWidget(shadow);
	
	layout->addWidget(makeAndroidToolButton(base, this, SLOT(clearSlot()),
		"New", 			"data/images/48x48/document-new.png", btnWidth));
	
	layout->addWidget(makeAndroidToolButton(base, this, SLOT(loadSlot()),
		"Open", 		"data/images/48x48/document-open.png", btnWidth));
	
	layout->addWidget(makeAndroidToolButton(base, this, SLOT(saveSlot()),
		"Save As...", 	 	"data/images/48x48/document-save.png", btnWidth));
	
	layout->addWidget(makeAndroidToolButton(base, this, SLOT(chooseBgSlot()),
		"Background...",	"data/images/48x48/document-save-as.png", btnWidth));
	
	layout->addWidget(makeAndroidToolButton(base, this, SLOT(toggleApMarkMode()),
		m_scene->markApMode() ? "Disable Mark AP Mode" : "Mark AP Location",	
		"data/images/48x48/go-jump.png", btnWidth));
		
	layout->addWidget(makeAndroidToolButton(base, this, SLOT(prefsSlot()),
		"Options...", 		"data/images/48x48/document-properties.png", btnWidth));

	base->show();
	
}

void MapWindow::toggleApMarkMode()
{
	m_scene->setMarkApMode(!m_scene->markApMode());
}
