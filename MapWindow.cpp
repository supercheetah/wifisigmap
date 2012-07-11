#include "MapWindow.h"

#include "MapGraphicsScene.h"

#include <QTcpSocket>

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
					fprintf(stderr, "Debug: %s\n", msg);
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
	: QWidget(parent)
{
	#ifdef CUSTOM_MSG_HANDLER
		qt_debugSocket = new QTcpSocket(this);
		//qt_debugSocket->connectToHost("192.168.0.105", 3729);
		qt_debugSocket->connectToHost("10.10.9.90", 3729);
		qt_origMsgHandler = qInstallMsgHandler(myMessageOutput);
	#endif
	
	setWindowTitle("WiFi Signal Mapper");
	setupUi();
	
	/// NOTE just for testing
	//m_scene->loadResults("wmz/phc-firsttest.wmz");
	//m_scene->loadResults("wmz/test.wmz");
	//m_scene->loadResults("wmz/phcfirstrun.wmz");
	m_scene->loadResults("wmz/foobar.wmz");
}

#define makeButton2(object,layout,title,slot) \
	{ QPushButton *btn = new QPushButton(title); \
		connect(btn, SIGNAL(clicked()), object, slot); \
		layout->addWidget(btn); \
	} 
	
#define makeButton(layout,title,slot) \
	makeButton2(this,layout,title,slot) 
	
void MapWindow::setupUi()
{
	m_gv = new MapGraphicsView();
	
	QVBoxLayout *vbox = new QVBoxLayout(this);
	
	QHBoxLayout *hbox;
	hbox = new QHBoxLayout();
	
	makeButton(hbox,"New",SLOT(clearSlot()));
	makeButton(hbox,"Load",SLOT(loadSlot()));
	makeButton(hbox,"Save",SLOT(saveSlot()));
	
	#ifndef Q_OS_ANDROID
	// Disable on android because until I rewrite the file browser dialog,
	// the file browser is pretty much useless
	makeButton(hbox,"Background...",SLOT(chooseBgSlot()));
	#endif
	
	makeButton2(m_gv, hbox, "+", SLOT(zoomIn()));
	makeButton2(m_gv, hbox, "-", SLOT(zoomOut()));
	
	vbox->addLayout(hbox);
	
	vbox->addWidget(m_gv);
	
	m_scene = new MapGraphicsScene(this);
	m_gv->setScene(m_scene);
	
	hbox = new QHBoxLayout();
	m_apButton = new QPushButton("Mark AP");
	m_apButton->setCheckable(true);
	m_apButton->setChecked(false);
	connect(m_apButton, SIGNAL(toggled(bool)), m_scene, SLOT(setMarkApMode(bool)));
	hbox->addWidget(m_apButton);
	
	m_statusMsg = new QLabel("");
	hbox->addWidget(m_statusMsg);
	
	hbox->addStretch(1);
	
	QPushButton *prefs = new QPushButton(QPixmap(":/data/images/stock-preferences.png"), "");
	connect(prefs, SIGNAL(clicked()), this, SLOT(prefsSlot()));
	hbox->addWidget(prefs);
	
	connect(&m_statusClearTimer, SIGNAL(timeout()), this, SLOT(clearStatusMessage()));
	m_statusClearTimer.setSingleShot(true);
	
	vbox->addLayout(hbox);

	m_gv->setFocus();
}

void MapWindow::prefsSlot()
{
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
	
}

void MapWindow::setStatusMessage(const QString& msg, int timeout)
{
	#ifdef Q_OS_ANDROID
	m_statusMsg->setText("<font size='-1'>"+msg+"</b>");
	#else
	m_statusMsg->setText("<b>"+msg+"</b>");
	#endif
	
	if(timeout>0)
	{
		if(m_statusClearTimer.isActive())
			m_statusClearTimer.stop();
			
		m_statusClearTimer.setInterval(timeout);
		m_statusClearTimer.start();
	}
}

void MapWindow::clearStatusMessage()
{
	setStatusMessage("");
}

void MapWindow::flagApModeCleared()
{
	setStatusMessage("");
	m_apButton->setChecked(false);
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
	}
}

void MapWindow::loadSlot()
{
	/// TODO just for debugging till the filedialog works better on android
	#ifdef Q_OS_ANDROID
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
	#ifdef Q_OS_ANDROID
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
	m_scene->clear();
}



