#include "MapWindow.h"

//#define DEBUG_WIFI_FILE "scan3.txt"

// Empty #define makes it use live data
#define DEBUG_WIFI_FILE

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
	
// 	// Magic numbers - just fitting approx to view
// 	scaleView(1.41421);
// 	scaleView(1.41421);
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


MapWindow::MapWindow(QWidget *parent)
	: QWidget(parent)
{
	setWindowTitle("WiFi Signal Mapper");
	setupUi();
}

#define makeButton(layout,title,slot) \
	{ QPushButton *btn = new QPushButton(title); \
		connect(btn, SIGNAL(clicked()), this, slot); \
		layout->addWidget(btn); \
	} 
	
void MapWindow::setupUi()
{
	QVBoxLayout *vbox = new QVBoxLayout(this);
	
	QHBoxLayout *hbox;
	hbox = new QHBoxLayout();
	
	makeButton(hbox,"New",SLOT(clearSlot()));
	makeButton(hbox,"Load",SLOT(loadSlot()));
	makeButton(hbox,"Save",SLOT(saveSlot()));
	makeButton(hbox,"Background...",SLOT(chooseBgSlot()));
	
	vbox->addLayout(hbox);
	
	MapGraphicsView *gv = new MapGraphicsView();
	vbox->addWidget(gv);
	
	m_scene = new MapGraphicsScene(this);
	gv->setScene(m_scene);
	
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
}

void MapWindow::setStatusMessage(const QString& msg, int timeout)
{
	m_statusMsg->setText("<b>"+msg+"</b>");
	
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
	QString curFile = m_scene->currentMapFilename();
	if(curFile.trimmed().isEmpty())
		curFile = QSettings("wifisigmap").value("last-map-file","").toString();

	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Results"), curFile, tr("Wifi Signal Map (*.wmz);;Any File (*.*)"));
	if(fileName != "")
	{
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
	}
}

void MapWindow::saveSlot()
{
	QString curFile = m_scene->currentMapFilename();
	if(curFile.trimmed().isEmpty())
		curFile = QSettings("wifisigmap").value("last-map-file","").toString();

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Results"), curFile, tr("Wifi Signal Map (*.wmz);;Any File (*.*)"));
	if(fileName != "")
	{
		QFileInfo info(fileName);
		//if(info.suffix().isEmpty())
			//fileName += ".dviz";
		QSettings("wifisigmap").setValue("last-map-file",fileName);
		m_scene->saveResults(fileName);
// 		return true;
	}

}

void MapWindow::clearSlot()
{
	m_scene->clear();
}


void MapGraphicsScene::clear()
{
	m_sigValues.clear();
	QGraphicsScene::clear();
	
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
	
	m_bgFilename = "phc-floorplan/phc-floorplan-blocks.png";
	m_bgPixmap = QPixmap(m_bgFilename);
	
	m_bgPixmapItem = addPixmap(m_bgPixmap);
	m_bgPixmapItem->setZValue(0);
	
	connect(&m_longPressTimer, SIGNAL(timeout()), this, SLOT(longPressTimeout()));
	m_longPressTimer.setInterval(1000);
	m_longPressTimer.setSingleShot(true);
	
	addSigMapItem();
	
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
			
			QList<WifiDataResult> results = m_scanIf.scanWifi(DEBUG_WIFI_FILE);
			
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
						QTimer::singleShot(0, this, SLOT(-()));
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
			QList<WifiDataResult> results = m_scanIf.scanWifi(DEBUG_WIFI_FILE);
			
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
	
	const int iconSize = 32;
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
	
	QFont font("", (int)(iconSize *.33), QFont::Bold);
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
			int textY = (int)(iconRect.height()/2 - textRect.height()/2 + font.pointSizeF()*1.3); // 1.3 is just a cosmetic adjustment to center it better
			
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
	//if(origSize.isEmpty())
#ifdef Q_OS_ANDROID
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

void MapGraphicsScene::saveResults(QString filename)
{
	QSettings data(filename, QSettings::IniFormat);
		
	//beginWriteArray
	
	// TODO
	
}

void MapGraphicsScene::loadResults(QString filename)
{
	QSettings data(filename, QSettings::IniFormat);
		
	//beginReadArray
	
	// TODO
	
}




