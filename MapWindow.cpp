#include "MapWindow.h"

#ifdef Q_OS_ANDROID
	// Empty #define makes it use live data
	#define DEBUG_WIFI_FILE
#else
	#define DEBUG_WIFI_FILE "scan3.txt"
#endif

/// Just for testing on linux, defined after DEBUG_WIFI_FILE so we still can use cached data
#define Q_OS_ANDROID

MapGraphicsView::MapGraphicsView()
	: QGraphicsView()
{
	srand ( time(NULL) );
	
	setCacheMode(CacheBackground);
	//setViewportUpdateMode(BoundingRectViewportUpdate);
	//setRenderHint(QPainter::Antialiasing);
	setTransformationAnchor(AnchorUnderMouse);
	setResizeAnchor(AnchorViewCenter);
	setDragMode(QGraphicsView::ScrollHandDrag);
	
	setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );
	// if there are ever graphic glitches to be found, remove this again
	setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing | QGraphicsView::DontClipPainter | QGraphicsView::DontSavePainterState);

	//setCacheMode(QGraphicsView::CacheBackground);
	//setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
	setOptimizationFlags(QGraphicsView::DontSavePainterState);
	
	#ifdef Q_OS_ANDROID
	setFrameStyle(QFrame::NoFrame);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	#endif
}

void MapGraphicsView::zoomIn()
{
	scaleView(qreal(1.2));
}

void MapGraphicsView::zoomOut()
{
	scaleView(1 / qreal(1.2));
}

void MapGraphicsView::keyPressEvent(QKeyEvent *event)
{
	if(event->modifiers() & Qt::ControlModifier)
	{
		switch (event->key())
		{
			case Qt::Key_Plus:
				scaleView(qreal(1.2));
				break;
			case Qt::Key_Minus:
			case Qt::Key_Equal:
				scaleView(1 / qreal(1.2));
				break;
			default:
				QGraphicsView::keyPressEvent(event);
		}
	}
}


void MapGraphicsView::wheelEvent(QWheelEvent *event)
{
	scaleView(pow((double)2, event->delta() / 240.0));
}

void MapGraphicsView::scaleView(qreal scaleFactor)
{
	//qDebug() << "MapGraphicsView::scaleView: "<<scaleFactor;
	
	qreal factor = matrix().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
	//qDebug() << "Scale factor:" <<factor;
	if (factor < 0.001 || factor > 100)
		return;
	
	scale(scaleFactor, scaleFactor);
}

void MapGraphicsView::mouseMoveEvent(QMouseEvent * mouseEvent)
{
	qobject_cast<MapGraphicsScene*>(scene())->invalidateLongPress();
		
	QGraphicsView::mouseMoveEvent(mouseEvent);
}


void MapGraphicsScene::setMarkApMode(bool flag)
{
	m_markApMode = flag;
	if(flag)
		m_mapWindow->setStatusMessage("Touch and hold map to mark AP location");
	else
		m_mapWindow->flagApModeCleared(); // resets push button state as well
}


// #define CUSTOM_MSG_HANDLER
// 
// #if defined(CUSTOM_MSG_HANDLER)
// 
// 	#if defined(Q_OS_WIN)
// 	extern Q_CORE_EXPORT void qWinMsgHandler(QtMsgType t, const char* str);
// 	#endif
// 
// 	static QtMsgHandler qt_origMsgHandler = 0;
// 
// 	void myMessageOutput(QtMsgType type, const char *msg)
// 	{
// 		#if defined(Q_OS_WIN)
// 		if (!qt_origMsgHandler)
// 			qt_origMsgHandler = qWinMsgHandler;
// 		#endif
// 
// 		switch (type)
// 		{
// 			case QtDebugMsg:
// 				//AppSettings::sendCheckin("/core/debug",QString(msg));
// 				if(qt_origMsgHandler)
// 					qt_origMsgHandler(type,msg);
// 				else
// 					fprintf(stderr, "Debug: %s\n", msg);
// 				break;
// 			case QtWarningMsg:
// 				//AppSettings::sendCheckin("/core/warn",QString(msg));
// 				if(qt_origMsgHandler)
// 					qt_origMsgHandler(QtDebugMsg,msg);
// 				else
// 					fprintf(stderr, "Warning: %s\n", msg);
// 				break;
// 			case QtCriticalMsg:
// 	// 			if(qt_origMsgHandler)
// 	// 				qt_origMsgHandler(type,msg);
// 	// 			else
// 	// 				fprintf(stderr, "Critical: %s\n", msg);
// 	// 			break;
// 			case QtFatalMsg:
// 				AppSettings::sendCheckin("/core/fatal",QString(msg));
// 				if(qt_origMsgHandler)
// 				{
// 					qt_origMsgHandler(QtDebugMsg,msg);
// 					//qt_origMsgHandler(type,msg);
// 				}
// 				else
// 				{
// 
// 					fprintf(stderr, "Fatal: %s\n", msg);
// 				}
// 				//QMessageBox::critical(0,"Fatal Error",msg);
// 				//qt_origMsgHandler(QtDebugMsg,msg);
// 				/*
// 				if(strstr(msg,"out of memory, returning null image") != NULL)
// 				{
// 					QPixmapCache::clear();
// 					qt_origMsgHandler(QtDebugMsg, "Attempted to clear QPixmapCache, continuing");
// 					return;
// 				}
// 				*/
// 				abort();
// 		}
// 	}
// #endif // CUSTOM_MSG_HANDLER
// 
// void AppSettings::initApp(const QString& appName)
// {
// 	#if defined(CUSTOM_MSG_HANDLER)
// 		qt_origMsgHandler = qInstallMsgHandler(myMessageOutput);
// 	#endif

MapWindow::MapWindow(QWidget *parent)
	: QWidget(parent)
{
	setWindowTitle("WiFi Signal Mapper");
	setupUi();
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
	
	connect(&m_statusClearTimer, SIGNAL(timeout()), this, SLOT(clearStatusMessage()));
	m_statusClearTimer.setSingleShot(true);
	
	vbox->addLayout(hbox);

	m_gv->setFocus();
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


void MapGraphicsScene::clear()
{
	m_apLocations.clear();
	m_sigValues.clear();
	QGraphicsScene::clear();
	
	m_bgFilename = ""; //"phc-floorplan/phc-floorplan-blocks.png";
	m_bgPixmap = QPixmap(2000,2000); //QApplication::desktop()->screenGeometry().size());
	m_bgPixmap.fill(Qt::white);
	QPainter p(&m_bgPixmap);
	p.setPen(QPen(Qt::gray,3.0));
	for(int x=0; x<m_bgPixmap.width(); x+=64)
	{
		p.drawLine(x,0,x,m_bgPixmap.height());
		for(int y=0; y<m_bgPixmap.height(); y+=64)
		{
			p.drawLine(0,y,m_bgPixmap.width(),y);
		}
	}
	
	m_bgPixmapItem = addPixmap(m_bgPixmap);
	m_bgPixmapItem->setZValue(0);
	
	addSigMapItem();
	
	m_currentMapFilename = "";
}

void MapGraphicsScene::setBgFile(QString filename)
{
	m_bgFilename = filename;
	m_bgPixmap = QPixmap(m_bgFilename);
	
	m_bgPixmapItem->setPixmap(m_bgPixmap);
}

MapGraphicsScene::MapGraphicsScene(MapWindow *map)
	: m_markApMode(false)
	, m_develTestMode(false)
	, m_mapWindow(map)
{
	//setBackgroundBrush(QImage("PCI-PlantLayout-20120705-2048px-adjusted.png"));
	//m_bgPixmap = QPixmap("PCI-PlantLayout-20120705-2048px-adjusted.png");
	
	connect(&m_longPressTimer, SIGNAL(timeout()), this, SLOT(longPressTimeout()));
	m_longPressTimer.setInterval(1000);
	m_longPressTimer.setSingleShot(true);
	
	clear(); // sets up background and other misc items

	qDebug() << "MapGraphicsScene: Setup and ready to go.";
	
// 	QSizeF sz = m_bgPixmap.size();
// 	double w = sz.width();
// 	double h = sz.height();
	
	// TODO just for debugging - TODO add in mode so user can specify AP locations
// 	addApMarker(QPointF(w*.1, h*.8), "TE:ST:MA:C1:23:00");
// 	addApMarker(QPointF(w*.6, h*.6), "TE:ST:MA:C1:23:01");
// 	addApMarker(QPointF(w*.8, h*.1), "TE:ST:MA:C1:23:02");
	
// 	addSignalMarker(QPointF(w*.50, h*.50), .99);
// 	addSignalMarker(QPointF(w*.75, h*.25), .5);
// 	addSignalMarker(QPointF(w*.70, h*.40), .7);
// 	addSignalMarker(QPointF(w*.80, h*.90), .4);
// 	addSignalMarker(QPointF(w*.20, h*.60), .6);
// 	addSignalMarker(QPointF(w*.20, h*.10), .3);
// 	
// 	renderSigMap();
	
	//m_scanIf.scanWifi();

	
}

void MapGraphicsScene::addApMarker(QPointF point, QString mac)
{
	m_apLocations[mac] = point;
	
	QImage markerGroup(":/data/images/ap-marker.png");
	
	markerGroup = addDropShadow(markerGroup, (double)(markerGroup.width()/2));
		
	//markerGroup.save("apMarkerDebug.png");
	
	QGraphicsItem *item = addPixmap(QPixmap::fromImage(markerGroup));
	
	double w2 = (double)(markerGroup.width())/2.;
	double h2 = (double)(markerGroup.height())/2.;
	QPointF pnt(point.x()-w2,point.y()-h2);
	item->setPos(QPointF(pnt));
	
// 	item->setFlag(QGraphicsItem::ItemIsMovable);
// 	item->setFlag(QGraphicsItem::ItemSendsGeometryChanges);
// 	item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	item->setZValue(99);
}


void MapGraphicsScene::addSigMapItem()
{
	QPixmap empty(m_bgPixmap.size());
	empty.fill(Qt::transparent);
	m_sigMapItem = addPixmap(empty);
	m_sigMapItem->setZValue(1);
	m_sigMapItem->setOpacity(0.75);
}
	
void MapGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent * mouseEvent)
{
	if(m_longPressTimer.isActive())
		m_longPressTimer.stop();
	m_longPressTimer.start();
	m_pressPnt = mouseEvent->lastScenePos();
	
	QGraphicsScene::mousePressEvent(mouseEvent);
}

void MapGraphicsScene::invalidateLongPress()
{
	if(m_longPressTimer.isActive())
		m_longPressTimer.stop();
}

void MapGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent * mouseEvent)
{
	invalidateLongPress();
		
	QGraphicsScene::mouseReleaseEvent(mouseEvent);
}

void MapGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent * mouseEvent)
{
	invalidateLongPress();
		
	QGraphicsScene::mouseMoveEvent(mouseEvent);
}

void MapGraphicsScene::longPressTimeout()
{
	if(!m_develTestMode)
	{
		if(m_markApMode)
		{
			/// Scan for APs nearby and prompt user to choose AP
			
			m_mapWindow->setStatusMessage(tr("<font color='green'>Scanning...</font>"));
			QList<WifiDataResult> results = m_scanIf.scanWifi(DEBUG_WIFI_FILE);
			m_mapWindow->setStatusMessage(tr("<font color='green'>Scan finished!</font>"), 3000);
			
			if(results.isEmpty())
			{
				m_mapWindow->flagApModeCleared();
				m_mapWindow->setStatusMessage(tr("<font color='red'>No APs found nearby</font>"), 3000);
			}
			else
			{
				QStringList items;
				foreach(WifiDataResult result, results)
					items << QString("%1%: %2 - %3").arg(QString().sprintf("%02d", (int)(result.value*100))).arg(result.mac/*.left(6)*/).arg(result.essid);
				
				qSort(items.begin(), items.end());
				
				// Reverse order so strongest appears on top
				QStringList tmp;
				for(int i=items.size()-1; i>-1; i--)
					tmp.append(items[i]);
				items = tmp;

				
				bool ok;
				QString item = QInputDialog::getItem(0, tr("Found APs"),
									tr("AP:"), items, 0, false, &ok);
				if (ok && !item.isEmpty())
				{
					QStringList pair = item.split(" ");
					if(pair.size() < 2) // problem getting MAC
						ok = false;
					else
					{
						// If user chooses AP, call addApMarker(QString mac, QPoint location);
						QString mac = pair[1];
						
						// Find result just to tell user ESSID
						WifiDataResult matchingResult;
						foreach(WifiDataResult result, results)
							if(result.mac == mac)
								matchingResult = result;
						
						// Add to map and ap list
						addApMarker(m_pressPnt, mac);
						
						// Update UI
						m_mapWindow->flagApModeCleared();
						m_mapWindow->setStatusMessage(tr("Added %1 (%2)").arg(mac).arg(matchingResult.valid ? matchingResult.essid : "Unknown ESSID"), 3000);
						
						// Render map overlay (because the AP may be tied to an existing scan result)
						QTimer::singleShot(0, this, SLOT(renderSigMap()));
						//renderSigMap();
					}
				}
				else
					ok = false;
				
				if(!ok)
				{
					m_mapWindow->flagApModeCleared();
					m_mapWindow->setStatusMessage(tr("User canceled"), 3000);
				}
			}
		}
		else
		/// Not in "mark AP" mode - regular scan mode, so scan and add store results
		{
			// scan for APs nearby
			m_mapWindow->setStatusMessage(tr("<font color='green'>Scanning...</font>"));
			QList<WifiDataResult> results = m_scanIf.scanWifi(DEBUG_WIFI_FILE);
			m_mapWindow->setStatusMessage(tr("<font color='green'>Scan finished!</font>"), 3000);
			
			if(results.size() > 0)
			{
				// call addSignalMarker() with presspnt
				addSignalMarker(m_pressPnt, results);
				
				m_mapWindow->setStatusMessage(tr("Added marker for %1 APs").arg(results.size()), 3000);
				
				// Render map overlay
				QTimer::singleShot(0, this, SLOT(renderSigMap()));
				//renderSigMap();
				
				QStringList notFound;
				foreach(WifiDataResult result, results)
					if(!m_apLocations.contains(result.mac))
						notFound << QString("<font color='%1'>%2</font> (<i>%3</i>)")
							.arg(colorForSignal(result.value, result.mac).name())
							.arg(result.mac.right(6))
							.arg(result.essid); // last two octets of mac should be enough
						
				if(notFound.size() > 0)
					m_mapWindow->setStatusMessage(tr("Ok, but %2 APs need marked: %1").arg(notFound.join(", ")).arg(notFound.size()), 10000);
			}
			else
			{
				m_mapWindow->setStatusMessage(tr("<font color='red'>No APs found nearby</font>"), 3000);
			}
		}
	}
	else
	{
		/// This is just development code - need to integrate with WifiDataCollector
	
		QList<WifiDataResult> results;
		for(int i=0; i<3; i++)
		{
			//const int range = 40;
			//int sig = (int)(rand() % range);
			
			QString apMac = QString("TE:ST:MA:C1:23:0%1").arg(i);
			
			QPointF center = m_apLocations[apMac];
			double dx = m_pressPnt.x() - center.x();
			double dy = m_pressPnt.y() - center.y();
			double distFromCenter = sqrt(dx*dx + dy*dy);
			
			double rangeMax = 2694.0; // TODO - This is the diagnal size of the test background
			
			double fakeSignalLevel = distFromCenter / (rangeMax * .25); // reduce for lower falloff
			
			const float range = 15.;
			float rv = ((float)(rand() % (int)range) - (range/2.)) / 100.;
		
			fakeSignalLevel += rv; // add jitter
			
			fakeSignalLevel = 1 - fakeSignalLevel; // invert level so further away from center is lower signal
		
			qDebug() << "MapGraphicsScene::longPressTimeout(): AP#"<<i<<": "<<apMac<<": dist: "<<distFromCenter<<", rv:"<<rv<<", fakeSignalLevel:"<<fakeSignalLevel;
			
			if(fakeSignalLevel > .05)
			{
				if(fakeSignalLevel > 1.)
					fakeSignalLevel = 1.;
					
				WifiDataResult result;
				result.value = fakeSignalLevel;
				result.essid = "Testing123";
				result.mac = apMac;
				result.valid = true;
				result.chan = (double)i;
				result.dbm = (int)(fakeSignalLevel*100) - 100;
				results << result;
			}
		}
				
		addSignalMarker(m_pressPnt, results);
		QTimer::singleShot(0, this, SLOT(renderSigMap()));
	}
}



class ImageFilters
{
public:
	static QImage blurred(const QImage& image, const QRect& rect, int radius);
	
	// Modifies 'image'
	static void blurImage(QImage& image, int radius, bool highQuality = false);
	static QRectF blurredBoundingRectFor(const QRectF &rect, int radius);
	static QSizeF blurredSizeFor(const QSizeF &size, int radius);

};

// Qt 4.6.2 includes a wonderfully optimized blur function in /src/gui/image/qpixmapfilter.cpp
// I'll just hook into their implementation here, instead of reinventing the wheel.
extern void qt_blurImage(QImage &blurImage, qreal radius, bool quality, int transposed = 0);

const qreal radiusScale = qreal(1.5);

// Copied from QPixmapBlurFilter::boundingRectFor(const QRectF &rect)
QRectF ImageFilters::blurredBoundingRectFor(const QRectF &rect, int radius) 
{
	const qreal delta = radiusScale * radius + 1;
	return rect.adjusted(-delta, -delta, delta, delta);
}

QSizeF ImageFilters::blurredSizeFor(const QSizeF &size, int radius)
{
	const qreal delta = radiusScale * radius + 1;
	QSizeF newSize(size.width()  + delta, 
	               size.height() + delta);
	
	return newSize;
}

// Modifies the input image, no copying
void ImageFilters::blurImage(QImage& image, int radius, bool highQuality)
{
	qt_blurImage(image, radius, highQuality);
}

// Blur the image according to the blur radius
// Based on exponential blur algorithm by Jani Huhtanen
// (maximum radius is set to 16)
QImage ImageFilters::blurred(const QImage& image, const QRect& /*rect*/, int radius)
{
	QImage copy = image.copy();
	qt_blurImage(copy, radius, false);
	return copy;
}


void MapGraphicsScene::addSignalMarker(QPointF point, QList<WifiDataResult> results)
{	
#ifdef Q_OS_ANDROID
	const int iconSize = 64;
#else
	const int iconSize = 32;
#endif

	const double iconSizeHalf = ((double)iconSize)/2.;
		
	int numResults = results.size();
	double angleStepSize = 360. / ((double)numResults);
	
	QRectF boundingRect;
	QList<QRectF> iconRects;
	for(int resultIdx=0; resultIdx<numResults; resultIdx++)
	{
		double rads = ((double)resultIdx) * angleStepSize * 0.0174532925;
		double iconX = iconSizeHalf/2 * numResults * cos(rads);
		double iconY = iconSizeHalf/2 * numResults * sin(rads);
		QRectF iconRect = QRectF(iconX - iconSizeHalf, iconY - iconSizeHalf, (double)iconSize, (double)iconSize);
		iconRects << iconRect;
		boundingRect |= iconRect;
	}
	
	boundingRect.adjust(-1,-1,+2,+2);
	
	qDebug() << "MapGraphicsScene::addSignalMarker(): boundingRect:"<<boundingRect;
	
	QImage markerGroup(boundingRect.size().toSize(), QImage::Format_ARGB32_Premultiplied);
	memset(markerGroup.bits(), 0, markerGroup.byteCount());//markerGroup.fill(Qt::green);
	QPainter p(&markerGroup);
	
#ifdef Q_OS_ANDROID
	QFont font("", 6, QFont::Bold);
#else
	QFont font("", (int)(iconSize*.33), QFont::Bold);
#endif

	p.setFont(font);
	p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);

	
	double zeroAdjX = boundingRect.x() < 0. ? fabs(boundingRect.x()) : 0.0; 
	double zeroAdjY = boundingRect.y() < 0. ? fabs(boundingRect.y()) : 0.0;
	
	for(int resultIdx=0; resultIdx<numResults; resultIdx++)
	{
		WifiDataResult result = results[resultIdx];
		
		QColor centerColor = colorForSignal(result.value, result.mac);
		
		QRectF iconRect = iconRects[resultIdx];
		iconRect.translate(zeroAdjX,zeroAdjY);
		
		qDebug() << "MapGraphicsScene::addSignalMarker(): resultIdx:"<<resultIdx<<": iconRect:"<<iconRect<<", centerColor:"<<centerColor;
		
		p.save();
		{
			p.translate(iconRect.topLeft());
			
			// Draw inner gradient
			QRadialGradient rg(QPointF(iconSize/2,iconSize/2),iconSize);
			rg.setColorAt(0, centerColor/*.lighter(100)*/);
			rg.setColorAt(1, centerColor.darker(500));
			//p.setPen(Qt::black);
			p.setBrush(QBrush(rg));
			
			// Draw outline
			p.setPen(QPen(Qt::black,1.5));
			p.drawEllipse(0,0,iconSize,iconSize);
			
			// Render signal percentage
			QString sigString = QString("%1%").arg((int)(result.value * 100));
			
			// Calculate text location centered in icon
			QRect textRect = p.boundingRect(0, 0, INT_MAX, INT_MAX, Qt::AlignLeft, sigString);
			int textX = (int)(iconRect.width()/2  - textRect.width()/2  + iconSize*.1); // .1 is just a cosmetic adjustment to center it better
			int textY = (int)(iconRect.height()/2 - textRect.height()/2 + font.pointSizeF() + 16);

			// Outline text in black
			p.setPen(Qt::black);
			p.drawText(textX-1, textY-1, sigString);
			p.drawText(textX+1, textY-1, sigString);
			p.drawText(textX+1, textY+1, sigString);
			p.drawText(textX-1, textY+1, sigString);
			
			// Render text in white
			p.setPen(Qt::white);
			p.drawText(textX, textY, sigString);
			
			
			//p.restore();
			/*
			p.setPen(Qt::green);
			p.setBrush(QBrush());
			p.drawRect(marker.rect());
			*/
		}
		p.restore();
	}
	
	p.end();
	
	markerGroup = addDropShadow(markerGroup, (double)iconSize / 2.);
		
	//markerGroup.save("markerGroupDebug.jpg");
	
	QGraphicsItem *item = addPixmap(QPixmap::fromImage(markerGroup));
	
	double w2 = (double)(markerGroup.width())/2.;
	double h2 = (double)(markerGroup.height())/2.;
	QPointF pnt(point.x()-w2,point.y()-h2);
	item->setPos(QPointF(pnt));
	
// 	item->setFlag(QGraphicsItem::ItemIsMovable);
// 	item->setFlag(QGraphicsItem::ItemSendsGeometryChanges);
// 	item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	item->setZValue(99);
	
	m_sigValues << new SigMapValue(point, results);

}

QImage MapGraphicsScene::addDropShadow(QImage markerGroup, double shadowSize)
{
	// Add in drop shadow
	if(shadowSize > 0.0)
	{
// 		double shadowOffsetX = 0.0;
// 		double shadowOffsetY = 0.0;
		QColor shadowColor = Qt::black;
		
		// create temporary pixmap to hold a copy of the text
		QSizeF blurSize = ImageFilters::blurredSizeFor(markerGroup.size(), (int)shadowSize);
		//qDebug() << "Blur size:"<<blurSize<<", doc:"<<doc.size()<<", shadowSize:"<<shadowSize;
		QImage tmpImage(blurSize.toSize(),QImage::Format_ARGB32_Premultiplied);
		memset(tmpImage.bits(),0,tmpImage.byteCount()); // init transparent
		
		// render the text
		QPainter tmpPainter(&tmpImage);
		
		tmpPainter.save();
		tmpPainter.translate(shadowSize, shadowSize);
		tmpPainter.drawImage(0,0,markerGroup);
		tmpPainter.restore();
		
		// blacken the image by applying a color to the copy using a QPainter::CompositionMode_DestinationIn operation. 
		// This produces a homogeneously-colored pixmap.
		QRect rect = tmpImage.rect();
		tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		tmpPainter.fillRect(rect, shadowColor);
		tmpPainter.end();
	
		// blur the colored image
		ImageFilters::blurImage(tmpImage, (int)shadowSize);
		
		// Render the original image back over the shadow
		tmpPainter.begin(&tmpImage);
		tmpPainter.save();
// 		tmpPainter.translate(shadowOffsetX - shadowSize,
// 				     shadowOffsetY - shadowSize);
		tmpPainter.translate(shadowSize, shadowSize);
		tmpPainter.drawImage(0,0,markerGroup);
		tmpPainter.restore();
		
		markerGroup = tmpImage;
	}
	
	return markerGroup;
}

bool SigMapValue::hasAp(QString mac)
{
	foreach(WifiDataResult result, scanResults)
		if(result.mac == mac)
			return true;
	
	return false;
}

double SigMapValue::signalForAp(QString mac, bool returnDbmValue)
{
	foreach(WifiDataResult result, scanResults)
		if(result.mac == mac)
			return returnDbmValue ?
				result.dbm  :
				result.value;
	
	return 0.0;
}


void MapGraphicsScene::renderSigMap()
{
	QSize origSize = m_bgPixmap.size();

#ifdef Q_OS_ANDROID
	if(origSize.isEmpty())
		origSize = QApplication::desktop()->screenGeometry().size();
#endif

// 	QSize renderSize = origSize;
// 	// Scale size down 1/3rd just so the renderTriangle() doesn't have to fill as many pixels
// 	renderSize.scale((int)(origSize.width() * .33), (int)(origSize.height() * .33), Qt::KeepAspectRatio);
// 	
// 	double dx = ((double)renderSize.width())  / ((double)origSize.width());
// 	double dy = ((double)renderSize.height()) / ((double)origSize.height());
// 	
// 	QImage mapImg(renderSize, QImage::Format_ARGB32_Premultiplied);
	QImage mapImg(origSize, QImage::Format_ARGB32_Premultiplied);
	QPainter p(&mapImg);
	p.fillRect(mapImg.rect(), Qt::transparent);
	//p.end();
	

	QHash<QString,QString> apMacToEssid;
	
	foreach(SigMapValue *val, m_sigValues)
		foreach(WifiDataResult result, val->scanResults)
			if(!apMacToEssid.contains(result.mac))
				apMacToEssid.insert(result.mac, result.essid);
	
	qDebug() << "MapGraphicsScene::renderSigMap(): Unique MACs: "<<apMacToEssid.keys()<<", mac->essid: "<<apMacToEssid;

	foreach(QString apMac, apMacToEssid.keys())
	{
		if(!m_apLocations.contains(apMac))
		{
			qDebug() << "MapGraphicsScene::renderSigMap(): Unable to render signals for apMac:"<<apMac<<", AP not marked on MAP yet.";
			continue;
		}
		
		QPointF center = m_apLocations[apMac];
		
		QRadialGradient rg;
		double maxDistFromCenter = -1;
		QPointF maxPoint; // not used yet, if at all...
		
		QColor color = colorForSignal(1.0, apMac);
		rg.setColorAt(0., color);
		qDebug() << "MapGraphicsScene::renderSigMap(): "<<apMac<<": sig:"<<1.0<<", color:"<<color;
		
		foreach(SigMapValue *val, m_sigValues)
		{
			if(val->hasAp(apMac))
			{
				double sig = val->signalForAp(apMac);
				QColor color = colorForSignal(sig, apMac);
				rg.setColorAt(1-sig, color);
				
				qDebug() << "MapGraphicsScene::renderSigMap(): "<<apMac<<": sig:"<<sig<<", color:"<<color;
				
				double dx = val->point.x() - center.x();
				double dy = val->point.y() - center.y();
				double distFromCenter = sqrt(dx*dx + dy*dy);
				if(distFromCenter > maxDistFromCenter)
				{
					maxDistFromCenter = distFromCenter;
					maxPoint = val->point;
				}
			}
		}
		
		rg.setCenter(center);
		rg.setFocalPoint(center);
		rg.setRadius(maxDistFromCenter);
		
		p.setOpacity(.75);
		p.setBrush(QBrush(rg));
		//p.setBrush(Qt::green);
		p.setPen(QPen(Qt::black,1.5));
		//p.drawEllipse(0,0,iconSize,iconSize);
		p.drawEllipse(center, maxDistFromCenter, maxDistFromCenter);
		qDebug() << "MapGraphicsScene::renderSigMap(): "<<apMac<<": center:"<<center<<", maxDistFromCenter:"<<maxDistFromCenter;
	}
	
	//mapImg.save("/tmp/mapImg.jpg");

	//m_sigMapItem->setPixmap(QPixmap::fromImage(mapImg.scaled(origSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
	m_sigMapItem->setPixmap(QPixmap::fromImage(mapImg));
}

QColor MapGraphicsScene::colorForSignal(double sig, QString apMac)
{
	//static int colorCounter = 16;
	QList<QColor> colorList;
	if(!m_colorListForMac.contains(apMac))
	{
		QColor baseColor = QColor::fromHsv(qrand() % 359, 255, 255);
		
		// paint the gradient
		QImage signalLevelImage(100,10,QImage::Format_ARGB32_Premultiplied);
		QPainter sigPainter(&signalLevelImage);
		QLinearGradient fade(QPoint(0,0),QPoint(100,0));
// 		fade.setColorAt( 0.3, Qt::black  );
// 		fade.setColorAt( 1.0, Qt::white  );
		fade.setColorAt( 0.3, Qt::red    );
		fade.setColorAt( 0.7, Qt::yellow );
		fade.setColorAt( 1.0, Qt::green  );
		sigPainter.fillRect( signalLevelImage.rect(), fade );
		//sigPainter.setCompositionMode(QPainter::CompositionMode_HardLight);
		sigPainter.setCompositionMode(QPainter::CompositionMode_Difference);
		sigPainter.fillRect( signalLevelImage.rect(), baseColor ); 
		
		sigPainter.end();
		
		// just for debugging (that's why the img is 10px high - so it is easier to see in this output)
		//signalLevelImage.save(tr("signalLevelImage-%1.png").arg(apMac));
		
		QRgb* scanline = (QRgb*)signalLevelImage.scanLine(0);
		for(int i=0; i<100; i++)
		{
			QColor color;
			color.setRgba(scanline[i]);
			colorList << color;
		}
		
		m_colorListForMac.insert(apMac, colorList);
	}
	else
	{
		colorList = m_colorListForMac.value(apMac); 
	}
	
	int colorIdx = (int)(sig * 100);
	if(colorIdx > 99)
		colorIdx = 99;
	if(colorIdx < 0)
		colorIdx = 0;
	
	return colorList[colorIdx];
}


QString qPointFToString(QPointF point)
{
	return QString("%1,%2").arg(point.x()).arg(point.y());
}

QPointF qPointFFromString(QString string)
{
	QStringList list = string.split(",");
	if(list.length() < 2)
		return QPointF();
	return QPointF(list[0].toDouble(),list[1].toDouble());
}

void MapGraphicsScene::saveResults(QString filename)
{
	QSettings data(filename, QSettings::IniFormat);
		
	qDebug() << "MapGraphicsScene::saveResults(): Saving to: "<<filename;
	
	// Store bg filename
	data.setValue("background", m_bgFilename);
	
	int idx = 0;
	
	// Save AP locations
	data.beginWriteArray("ap-locations");
	foreach(QString apMac, m_apLocations.keys())
	{
		QPointF center = m_apLocations[apMac];
		QList<QColor> colorList = m_colorListForMac.value(apMac);
		
		data.setArrayIndex(idx++);
		data.setValue("mac", apMac);
		data.setValue("center", qPointFToString(center));
		
		int colorIdx = 0;
		data.beginWriteArray("colors");
		foreach(QColor color, colorList)
		{
			data.setArrayIndex(colorIdx++);
			data.setValue("color", color.name());
		}
		data.endArray();
	}
	data.endArray();
		
	// Save signal readings
	//m_sigValues << new SigMapValue(point, results);
	idx = 0;
	data.beginWriteArray("readings");
	foreach(SigMapValue *val, m_sigValues)
	{
		data.setArrayIndex(idx++);
		data.setValue("point", qPointFToString(val->point));
		
		int resultIdx = 0;
		data.beginWriteArray("signals");
		foreach(WifiDataResult result, val->scanResults)
		{
			data.setArrayIndex(resultIdx++);
			foreach(QString key, result.rawData.keys())
			{
				data.setValue(key, result.rawData.value(key));
			}
		}
		data.endArray();
	}
	data.endArray();
}

void MapGraphicsScene::loadResults(QString filename)
{
	QSettings data(filename, QSettings::IniFormat);
	qDebug() << "MapGraphicsScene::loadResults(): Loading from: "<<filename;
	
	clear(); // clear and reset the map
	
	// Load background file
	QString bg = data.value("background").toString();
	if(!bg.isEmpty())
		setBgFile(bg);	
	
	// Load ap locations
	int numApLocations = data.beginReadArray("ap-locations");
	for(int i=0; i<numApLocations; i++)
	{
		data.setArrayIndex(i);
		
		QString apMac = data.value("mac").toString();
		QPointF center = qPointFFromString(data.value("center").toString());
		addApMarker(center, apMac);
	
		QList<QColor> colorList;
		
		int numColors = data.beginReadArray("colors");
		for(int j=0; j<numColors; j++)
		{
			data.setArrayIndex(j);
			QString color = data.value("color").toString();
			colorList << QColor(color);
			
		}
		data.endArray();
		
		m_colorListForMac[apMac] = colorList;
	}
	data.endArray();
	
	// Load signal readings
	int numReadings = data.beginReadArray("readings");
	for(int i=0; i<numReadings; i++)
	{
		data.setArrayIndex(i);
		QPointF point = qPointFFromString(data.value("point").toString());
		
		int numSignals = data.beginReadArray("signals");
		
		QList<WifiDataResult> results;
		for(int j=0; j<numSignals; j++)
		{
			data.setArrayIndex(j);
			QStringList keys = data.childKeys();
			QStringHash rawData;
			
			foreach(QString key, keys)
			{
				rawData[key] = data.value(key).toString();
			}
			
			WifiDataResult result;
			result.loadRawData(rawData);
			results << result;
		}
		data.endArray();
		
		addSignalMarker(point, results);
	}
	data.endArray();
	
	// Call for render of map overlay
	QTimer::singleShot(0, this, SLOT(renderSigMap()));
}




