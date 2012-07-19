#include "MapGraphicsScene.h"
#include "MapWindow.h"

#ifdef Q_OS_ANDROID
#include <QSensor>
#include <QSensorReading>
#endif

// Some math constants...
static const double Pi  = 3.14159265358979323846264338327950288419717;
static const double Pi2 = Pi * 2.;

///// Just for testing on linux, defined after DEBUG_WIFI_FILE so we still can use cached data
//#define Q_OS_ANDROID

#ifdef Q_OS_ANDROID
#define QT_NO_OPENGL
#endif

#ifndef QT_NO_OPENGL
#include <QtOpenGL/QGLWidget>
#endif

// #ifdef Q_OS_ANDROID
	#define DEFAULT_RENDER_MODE MapGraphicsScene::RenderCircles
//	#define DEFAULT_RENDER_MODE MapGraphicsScene::RenderRectangles
// #else
// 	#define DEFAULT_RENDER_MODE MapGraphicsScene::RenderRadial
// #endif

#define qDrawTextC(p,x,y,string,c1,c2) 	\
	p.setPen(c1);			\
	p.drawText(x-1, y-1, string);	\
	p.drawText(x+1, y-1, string);	\
	p.drawText(x+1, y+1, string);	\
	p.drawText(x-1, y+1, string);	\
	p.setPen(c2);			\
	p.drawText(x, y, string);	\
	
#define qDrawTextO(p,x,y,string) 	\
	qDrawTextC(p,x,y,string,Qt::black,Qt::white);

bool MapGraphicsScene_sort_WifiDataResult(WifiDataResult ra, WifiDataResult rb)
{
	return ra.value > rb.value;
}




static QString SigMapRenderer_sort_apMac;
static QPointF SigMapRenderer_sort_center;

bool SigMapRenderer_sort_SigMapValue_bySignal(SigMapValue *a, SigMapValue *b)
{
	QString apMac  = SigMapRenderer_sort_apMac;
	if(!a || !b) return false;

	double va = a && a->hasAp(apMac) ? a->signalForAp(apMac) : 0.;
	double vb = b && b->hasAp(apMac) ? b->signalForAp(apMac) : 0.;
	return va < vb;

}

bool SigMapRenderer_sort_SigMapValue(SigMapValue *a, SigMapValue *b)
{
	QString apMac  = SigMapRenderer_sort_apMac;
	QPointF center = SigMapRenderer_sort_center;

// 	double va = a && a->hasAp(apMac) ? a->signalForAp(apMac) : 0.;
// 	double vb = b && b->hasAp(apMac) ? b->signalForAp(apMac) : 0.;
// 	return va < vb;

	double aDist = 0.0;
	double bDist = 0.0;

	// sqrt() commented out because its not needed just for sorting (e.g. a squared value compares the same as a sqrt'ed value)
	if(a)
	{
		if(a->renderDataDirty)
		{
			QPointF calcPoint = a->point - center;
			aDist = /*sqrt*/(calcPoint.x() * calcPoint.x() + calcPoint.y() * calcPoint.y());

			a->renderLevel     = aDist;
			a->renderDataDirty = false;
		}
		else
		{
			aDist = a->renderLevel;
		}
	}

	if(b)
	{
		if(b->renderDataDirty)
		{
			QPointF calcPoint = b->point - center;
			bDist = /*sqrt*/(calcPoint.x() * calcPoint.x() + calcPoint.y() * calcPoint.y());

			b->renderLevel     = bDist;
			b->renderDataDirty = false;
		}
		else
		{
			bDist = b->renderLevel;
		}
	}

	return aDist > bDist;
}


/// fillTriColor() is a routine I translated from Delphi, atrribution below.

class TRGBFloat {
public:
	float R;
	float G;
	float B;
};

bool fillTriColor(QImage *img, QVector<QPointF> points, QList<QColor> colors)
{
	if(!img)
		return false;
		
	if(img->format() != QImage::Format_ARGB32_Premultiplied &&
	   img->format() != QImage::Format_ARGB32)
		return false;
		
	double imgWidth  = img->width();
	double imgHeight = img->height();
	
	/// The following routine translated from Delphi, originally found at:
	// http://www.swissdelphicenter.ch/en/showcode.php?id=1780
	
	double  LX, RX, Ldx, Rdx, Dif1, Dif2;
	
	TRGBFloat LRGB, RRGB, RGB, RGBdx, LRGBdy, RRGBdy;
		
	QColor RGBT;
	double y, x, ScanStart, ScanEnd;// : integer;
	bool Right;
	QPointF tmpPoint;
	QColor tmpColor;
	
	
	 // sort vertices by Y
	int Vmax = 0;
	if (points[1].y() > points[0].y())
		Vmax = 1;
	if (points[2].y() > points[Vmax].y())
		 Vmax = 2;
		 
	if(Vmax != 2)
	{
		tmpPoint = points[2];
		points[2] = points[Vmax];
		points[Vmax] = tmpPoint;
		
		tmpColor = colors[2];
		colors[2] = colors[Vmax];
		colors[Vmax] = tmpColor;
	}
		
	if(points[1].y() > points[0].y())
		Vmax = 1;
	else 
		Vmax = 0;
		
	if(Vmax == 0)
	{
		tmpPoint = points[1];
		points[1] = points[0];
		points[0] = tmpPoint;
		
		tmpColor = colors[1];
		colors[1] = colors[0];
		colors[0] = tmpColor;
	}
	
	Dif1 = points[2].y() - points[0].y();
	if(Dif1 == 0)
		Dif1 = 0.001; // prevent div by 0 error
	
	Dif2 = points[1].y() - points[0].y();
	if(Dif2 == 0)
		Dif2 = 0.001;

	//work out if middle point is to the left or right of the line
	// connecting upper and lower points
	if(points[1].x() > (points[2].x() - points[0].x()) * Dif2 / Dif1 + points[0].x())
		Right = true;
	else	Right = false;
	
	   // calculate increments in x and colour for stepping through the lines
	if(Right)
	{
		Ldx = (points[2].x() - points[0].x()) / Dif1;
		Rdx = (points[1].x() - points[0].x()) / Dif2;
		LRGBdy.B = (colors[2].blue()  - colors[0].blue())  / Dif1;
		LRGBdy.G = (colors[2].green() - colors[0].green()) / Dif1;
		LRGBdy.R = (colors[2].red()   - colors[0].red())   / Dif1;
		RRGBdy.B = (colors[1].blue()  - colors[0].blue())  / Dif2;
		RRGBdy.G = (colors[1].green() - colors[0].green()) / Dif2;
		RRGBdy.R = (colors[1].red()   - colors[0].red())   / Dif2;
	}
	else
	{
		Ldx = (points[1].x() - points[0].x()) / Dif2;
		Rdx = (points[2].x() - points[0].x()) / Dif1;
		
		RRGBdy.B = (colors[2].blue()  - colors[0].blue())  / Dif1;
		RRGBdy.G = (colors[2].green() - colors[0].green()) / Dif1;
		RRGBdy.R = (colors[2].red()   - colors[0].red())   / Dif1;
		LRGBdy.B = (colors[1].blue()  - colors[0].blue())  / Dif2;
		LRGBdy.G = (colors[1].green() - colors[0].green()) / Dif2;
		LRGBdy.R = (colors[1].red()   - colors[0].red())   / Dif2;
	}

	LRGB.R = colors[0].red();
	LRGB.G = colors[0].green();
	LRGB.B = colors[0].blue();
	RRGB = LRGB;
	
	LX = points[0].x() - 1; /* -1: fix for not being able to render in floating-point coorindates */
	RX = points[0].x();
	
	// fill region 1
	for(y = points[0].y(); y <= points[1].y() - 1; y++)
	{	
		// y clipping
		if(y > imgHeight - 1)
			break;
			
		if(y < 0)
		{
			LX = LX + Ldx;
			RX = RX + Rdx;
			LRGB.B = LRGB.B + LRGBdy.B;
			LRGB.G = LRGB.G + LRGBdy.G;
			LRGB.R = LRGB.R + LRGBdy.R;
			RRGB.B = RRGB.B + RRGBdy.B;
			RRGB.G = RRGB.G + RRGBdy.G;
			RRGB.R = RRGB.R + RRGBdy.R;
			continue;
		}
		
		// Scan = ABitmap.ScanLine[y];
		
		// calculate increments in color for stepping through pixels
		Dif1 = RX - LX + 1;
		if(Dif1 == 0)
			Dif1 = 0.001;
		RGBdx.B = (RRGB.B - LRGB.B) / Dif1;
		RGBdx.G = (RRGB.G - LRGB.G) / Dif1;
		RGBdx.R = (RRGB.R - LRGB.R) / Dif1;
		
		// x clipping
		if(LX < 0)
		{
			ScanStart = 0;
			RGB.B = LRGB.B + (RGBdx.B * fabs(LX));
			RGB.G = LRGB.G + (RGBdx.G * fabs(LX));
			RGB.R = LRGB.R + (RGBdx.R * fabs(LX));
		}
		else
		{
			RGB = LRGB;
			ScanStart = LX; //round(LX);
		} 
		
		// Was RX-1 - inverted to fix not being able to render in floatingpoint coords
		if(RX + 1 > imgWidth - 1)
			ScanEnd = imgWidth - 1;
		else	ScanEnd = RX + 1; //round(RX) - 1;
		

		// scan the line
		QRgb* scanline = (QRgb*)img->scanLine((int)y);
		for(x = ScanStart; x <= ScanEnd; x++)
		{
			scanline[(int)x] = qRgb((int)RGB.R, (int)RGB.G, (int)RGB.B);
			
			RGB.B = RGB.B + RGBdx.B;
			RGB.G = RGB.G + RGBdx.G;
			RGB.R = RGB.R + RGBdx.R;
		}
		
		// increment edge x positions
		LX = LX + Ldx;
		RX = RX + Rdx;
		
		// increment edge colours by the y colour increments
		LRGB.B = LRGB.B + LRGBdy.B;
		LRGB.G = LRGB.G + LRGBdy.G;
		LRGB.R = LRGB.R + LRGBdy.R;
		RRGB.B = RRGB.B + RRGBdy.B;
		RRGB.G = RRGB.G + RRGBdy.G;
		RRGB.R = RRGB.R + RRGBdy.R;
	}

	
	Dif1 = points[2].y() - points[1].y();
	if(Dif1 == 0)
		Dif1 = 0.001;
		
	// calculate new increments for region 2
	if(Right)
	{
		Rdx = (points[2].x() - points[1].x()) / Dif1;
		RX  = points[1].x();
		RRGBdy.B = (colors[2].blue()  - colors[1].blue())  / Dif1;
		RRGBdy.G = (colors[2].green() - colors[1].green()) / Dif1;
		RRGBdy.R = (colors[2].red()   - colors[1].red())   / Dif1;
		RRGB.R = colors[1].red();
		RRGB.G = colors[1].green();
		RRGB.B = colors[1].blue();
	}
	else
	{
		Ldx = (points[2].x() - points[1].x()) / Dif1;
		LX  = points[1].x();
		LRGBdy.B = (colors[2].blue()  - colors[1].blue())  / Dif1;
		LRGBdy.G = (colors[2].green() - colors[1].green()) / Dif1;
		LRGBdy.R = (colors[2].red()   - colors[1].red())   / Dif1;
		LRGB.R = colors[1].red();
		LRGB.G = colors[1].green();
		LRGB.B = colors[1].blue();
	}

	// fill region 2
	for(y = points[1].y(); y < points[2].y() - 1; y++)
	{
		// y clipping
		if(y > imgHeight - 1)
			break;
		if(y < 0)
		{
			LX = LX + Ldx;
			RX = RX + Rdx;
			LRGB.B = LRGB.B + LRGBdy.B;
			LRGB.G = LRGB.G + LRGBdy.G;
			LRGB.R = LRGB.R + LRGBdy.R;
			RRGB.B = RRGB.B + RRGBdy.B;
			RRGB.G = RRGB.G + RRGBdy.G;
			RRGB.R = RRGB.R + RRGBdy.R;
			continue;
		}
		
		//Scan := ABitmap.ScanLine[y];
		
		Dif1 = RX - LX + 1;
		if(Dif1 == 0)
			Dif1 = 0.001;
		
		RGBdx.B = (RRGB.B - LRGB.B) / Dif1;
		RGBdx.G = (RRGB.G - LRGB.G) / Dif1;
		RGBdx.R = (RRGB.R - LRGB.R) / Dif1;
		
		// x clipping
		if(LX < 0)
		{
			ScanStart = 0;
			RGB.B = LRGB.B + (RGBdx.B * fabs(LX));
			RGB.G = LRGB.G + (RGBdx.G * fabs(LX));
			RGB.R = LRGB.R + (RGBdx.R * fabs(LX));
		}
		else
		{
			RGB = LRGB;
			ScanStart = LX; //round(LX);
		}
		
		// Was RX-1 - inverted to fix not being able to render in floatingpoint coords
		if(RX + 1 > imgWidth - 1)
			ScanEnd = imgWidth - 1;
		else 	ScanEnd = RX + 1; //round(RX) - 1;
		
		// scan the line
		QRgb* scanline = (QRgb*)img->scanLine((int)y);
		for(x = ScanStart; x < ScanEnd; x++)
		{
			scanline[(int)x] = qRgb((int)RGB.R, (int)RGB.G, (int)RGB.B);
			
			RGB.B = RGB.B + RGBdx.B;
			RGB.G = RGB.G + RGBdx.G;
			RGB.R = RGB.R + RGBdx.R;
		}
		
		LX = LX + Ldx;
		RX = RX + Rdx;
		
		LRGB.B = LRGB.B + LRGBdy.B;
		LRGB.G = LRGB.G + LRGBdy.G;
		LRGB.R = LRGB.R + LRGBdy.R;
		RRGB.B = RRGB.B + RRGBdy.B;
		RRGB.G = RRGB.G + RRGBdy.G;
		RRGB.R = RRGB.R + RRGBdy.R;
	}
	
	return true;
}

/// MapGraphicsView class implementation

MapGraphicsView::MapGraphicsView()
	: QGraphicsView()
{
#ifndef Q_OS_WIN
	srand ( time(NULL) );
#endif
	m_scaleFactor = 1.;
	
	#ifndef QT_NO_OPENGL
	//setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
	#endif
	
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

void MapGraphicsView::reset()
{
	resetTransform();
	m_scaleFactor = 1.;
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
	
	m_scaleFactor *= scaleFactor;
	
	scale(scaleFactor, scaleFactor);
}

void MapGraphicsView::mouseMoveEvent(QMouseEvent * mouseEvent)
{
	qobject_cast<MapGraphicsScene*>(scene())->invalidateLongPress(mapToScene(mouseEvent->pos()));
		
	QGraphicsView::mouseMoveEvent(mouseEvent);
}

void MapGraphicsView::drawForeground(QPainter *p, const QRectF & /*upRect*/)
{
	MapGraphicsScene *gs = qobject_cast<MapGraphicsScene*>(scene());
	if(!gs)
		return;
	 
	int w = 150;
	int h = 150;
	int pad = 10;
	
	QRect rect(width() - w - pad, pad, w, h);
		
	p->save();
	p->resetTransform();
	p->setOpacity(1.);
	
	p->setPen(Qt::black);
	p->fillRect(rect, QColor(0,0,0,150));
	p->drawRect(rect);
	
	int fontSize = 10;// * (1/m_scaleFactor);
#ifdef Q_OS_ANDROID
	fontSize = 5;
#endif

	int margin = fontSize/2;
	int y = margin;
	int lineJump = (int)(fontSize * 1.33);

#ifdef Q_OS_ANDROID
	margin = fontSize;
	y = margin;
	lineJump = fontSize * 4;
#endif
	
	p->setFont(QFont("Monospace", fontSize, QFont::Bold));
	
	QList<WifiDataResult> results = gs->m_lastScanResults;
	
	// Sort not needed - already sorted when m_lastScanResults is set
	//qSort(results.begin(), results.end(), MapGraphicsScene_sort_WifiDataResult);
	
	foreach(WifiDataResult result, results)
	{
		QColor color = gs->colorForSignal(1.0, result.mac).lighter(100);
#ifdef Q_OS_ANDROID
		color = color.darker(300);
#endif
		QColor outline = Qt::white; //qGray(color.rgb()) < 60 ? Qt::white : Qt::black;
		QPoint pnt(rect.topLeft() + QPoint(margin, y += lineJump));
		qDrawTextC((*p), pnt.x(), pnt.y(), 
			QString( "%1% %2"  )
				.arg(QString().sprintf("%02d", (int)round(result.value * 100.)))
				.arg(result.essid),
			outline,
			color);
	}
	
	p->restore();
}

/// End MapGraphicsView implementation


/// Main class of this file - MapGraphicsScene

MapGraphicsScene::MapGraphicsScene(MapWindow *map)
	: QGraphicsScene()
	, m_colorCounter(0)
	, m_markApMode(false)
	, m_develTestMode(false)
	, m_mapWindow(map)
	, m_renderMode(DEFAULT_RENDER_MODE)
	, m_userItem(0)
{
	// Setup default render options
	m_renderOpts.cacheMapRender     = true;
	m_renderOpts.showReadingMarkers = true;
	m_renderOpts.multipleCircles	= false;
	m_renderOpts.fillCircles	= true;
	#ifdef Q_OS_ANDROID
	m_renderOpts.radialCircleSteps	= 4 * 4 * 2;
	m_renderOpts.radialLevelSteps	= (int)(100 / .2);
	#else
	m_renderOpts.radialCircleSteps	= 4 * 4 * 4;
	m_renderOpts.radialLevelSteps	= (int)(100 / .25);
	#endif
	m_renderOpts.radialAngleDiff	= 45 * 3;
	m_renderOpts.radialLevelDiff	= 100 / 3;
	m_renderOpts.radialLineWeight	= 200;

	// Create initial palette of colors for flagging APs
	m_masterColorsAvailable = QList<QColor>() 
		<< Qt::red 
		<< Qt::darkGreen 
		<< Qt::blue 
		<< Qt::magenta 
		<< "#0277fd" // cream blue 
		<< "#fd1402" // rich red
		<< "#f8fc0f" // kid yellow
		<< "#19f900" // kid green
		<< "#ff4500"  // dark orange
		;

	// Setup the "longpress" timer
	connect(&m_longPressTimer, SIGNAL(timeout()), this, SLOT(longPressTimeout()));
	m_longPressTimer.setInterval(500);
	m_longPressTimer.setSingleShot(true);
	
	// Setup the longpress "progress counter" (display progress to user)
	connect(&m_longPressCountTimer, SIGNAL(timeout()), this, SLOT(longPressCount()));
	m_longPressCountTimer.setInterval(m_longPressTimer.interval() / 10);
	m_longPressCountTimer.setSingleShot(false);
	
	// Setup rendering thread
	//qDebug() << "MapGraphicsScene::MapGraphicsScene(): currentThreadId:"<<QThread::currentThreadId();
	m_renderer = new SigMapRenderer(this);
	m_renderer->moveToThread(&m_renderingThread);
	connect(m_renderer, SIGNAL(renderProgress(double)), this, SLOT(renderProgress(double)));
	connect(m_renderer, SIGNAL(renderComplete(QPicture)), this, SLOT(renderComplete(QPicture)));
	//connect(m_renderer, SIGNAL(renderComplete(QImage)), this, SLOT(renderComplete(QImage)));
	m_renderingThread.start();
	
	// Setup the trigger timer
	connect(&m_renderTrigger, SIGNAL(timeout()), m_renderer, SLOT(render()));
	m_renderTrigger.setInterval(50);
	m_renderTrigger.setSingleShot(true); 
	
	// Received every time a scan is completed in continous mode (continuous mode being the default)
	connect(&m_scanIf, SIGNAL(scanFinished(QList<WifiDataResult>)), this, SLOT(scanFinished(QList<WifiDataResult>)));
	
	// Set up background and other misc items
	clear();
	
	QSettings settings("wifisigmap");
	m_showMyLocation       = settings.value("showMyLocation", true).toBool();
	m_autoGuessApLocations = settings.value("autoGuessApLocations", true).toBool();

	qDebug() << "MapGraphicsScene: Setup and ready to go.";
	
	QTimer::singleShot(1000, this, SLOT(debugTest()));
	
	#ifdef OPENCV_ENABLED
	m_kalman.predictionBegin(0,0);
	#endif

}

MapGraphicsScene::~MapGraphicsScene()
{
	if(m_renderer)
	{
		m_renderingThread.quit();
		delete m_renderer;
		m_renderer = 0;
	}
}


void MapGraphicsScene::debugTest()
{
	// Disable sensor testing for now - I'll work more on it later.
	return;

	#ifdef Q_OS_ANDROID
	QList<QByteArray> sensorList = QtMobility::QSensor::sensorTypes();
	qDebug() << "Sensor list length: "<<sensorList.size();
	foreach (QByteArray sensorName, sensorList)
	{
		qDebug() << "Sensor: "<<sensorName;
		QtMobility::QSensor *sensor = new QtMobility::QSensor(sensorName, this); // destroyed when this is destroyed
		sensor->setObjectName(sensorName);
		bool started = sensor->start();
		if(started)
		{
			qDebug() << " - Started!";
			connect(sensor, SIGNAL(readingChanged()), this, SLOT(sensorReadingChanged()));
		}
		else
			qDebug() << " ! Error starting";
	}

	/*
	// start the sensor
	QSensor sensor("QAccelerometer");
	sensor.start();

	// later
	QSensorReading *reading = sensor.reading();
	qreal x = reading->property("x").value<qreal>();
	qreal y = reading->value(1).value<qreal>();

	*/

	/// NOTE Starting QGeoPositionInfoSource causes the app to crash as of 2012-07-13
	/// Still crrashes as of 7/15
	/*
	QtMobility::QGeoPositionInfoSource *source = QtMobility::QGeoPositionInfoSource::createDefaultSource(this);
	if (source) {

	    //connect(source, SIGNAL(positionUpdated(QGeoPositionInfo)),
	    //	    this, SLOT(positionUpdated(QGeoPositionInfo)));

	    source->startUpdates();
	    qDebug() << "Started geo source:"<<source;
	    QtMobility::QGeoPositionInfo last = source->lastKnownPosition();
	    qDebug() << "Last position:" << last;
	}
	else
	    qDebug() << "No geo source found";
	*/

	#endif
}

#if 0

    /** Helper function to compute the angle change between two rotation matrices.
     *  Given a current rotation matrix (R) and a previous rotation matrix
     *  (prevR) computes the rotation around the x,y, and z axes which
     *  transforms prevR to R.
     *  outputs a 3 element vector containing the x,y, and z angle
     *  change at indexes 0, 1, and 2 respectively.
     * <p> Each input matrix is either as a 3x3 or 4x4 row-major matrix
     * depending on the length of the passed array:
     * <p>If the array length is 9, then the array elements represent this matrix
     * <pre>
     *   /  R[ 0]   R[ 1]   R[ 2]   \
     *   |  R[ 3]   R[ 4]   R[ 5]   |
     *   \  R[ 6]   R[ 7]   R[ 8]   /
     *</pre>
     * <p>If the array length is 16, then the array elements represent this matrix
     * <pre>
     *   /  R[ 0]   R[ 1]   R[ 2]   R[ 3]  \
     *   |  R[ 4]   R[ 5]   R[ 6]   R[ 7]  |
     *   |  R[ 8]   R[ 9]   R[10]   R[11]  |
     *   \  R[12]   R[13]   R[14]   R[15]  /
     *</pre>
     * @param R current rotation matrix
     * @param prevR previous rotation matrix
     * @param angleChange an array of floats in which the angle change is stored
     */

    public static void getAngleChange( float[] angleChange, float[] R, float[] prevR) {
	float rd1=0,rd4=0, rd6=0,rd7=0, rd8=0;
	float ri0=0,ri1=0,ri2=0,ri3=0,ri4=0,ri5=0,ri6=0,ri7=0,ri8=0;
	float pri0=0, pri1=0, pri2=0, pri3=0, pri4=0, pri5=0, pri6=0, pri7=0, pri8=0;
	int i, j, k;

	if(R.length == 9) {
	    ri0 = R[0];
	    ri1 = R[1];
	    ri2 = R[2];
	    ri3 = R[3];
	    ri4 = R[4];
	    ri5 = R[5];
	    ri6 = R[6];
	    ri7 = R[7];
	    ri8 = R[8];
	} else if(R.length == 16) {
	    ri0 = R[0];
	    ri1 = R[1];
	    ri2 = R[2];
	    ri3 = R[4];
	    ri4 = R[5];
	    ri5 = R[6];
	    ri6 = R[8];
	    ri7 = R[9];
	    ri8 = R[10];
	}

	if(prevR.length == 9) {
	    pri0 = R[0];
	    pri1 = R[1];
	    pri2 = R[2];
	    pri3 = R[3];
	    pri4 = R[4];
	    pri5 = R[5];
	    pri6 = R[6];
	    pri7 = R[7];
	    pri8 = R[8];
	} else if(prevR.length == 16) {
	    pri0 = R[0];
	    pri1 = R[1];
	    pri2 = R[2];
	    pri3 = R[4];
	    pri4 = R[5];
	    pri5 = R[6];
	    pri6 = R[8];
	    pri7 = R[9];
	    pri8 = R[10];
	}

	// calculate the parts of the rotation difference matrix we need
	// rd[i][j] = pri[0][i] * ri[0][j] + pri[1][i] * ri[1][j] + pri[2][i] * ri[2][j];

	rd1 = pri0 * ri1 + pri3 * ri4 + pri6 * ri7; //rd[0][1]
	rd4 = pri1 * ri1 + pri4 * ri4 + pri7 * ri7; //rd[1][1]
	rd6 = pri2 * ri0 + pri5 * ri3 + pri8 * ri6; //rd[2][0]
	rd7 = pri2 * ri1 + pri5 * ri4 + pri8 * ri7; //rd[2][1]
	rd8 = pri2 * ri2 + pri5 * ri5 + pri8 * ri8; //rd[2][2]

	angleChange[0] = (float)Math.atan2(rd1, rd4);
	angleChange[1] = (float)Math.asin(-rd7);
	angleChange[2] = (float)Math.atan2(-rd6, rd8);

    }

    /** Helper function to convert a rotation vector to a rotation matrix.
     *  Given a rotation vector (presumably from a ROTATION_VECTOR sensor), returns a
     *  9  or 16 element rotation matrix in the array R.  R must have length 9 or 16.
     *  If R.length == 9, the following matrix is returned:
     * <pre>
     *   /  R[ 0]   R[ 1]   R[ 2]   \
     *   |  R[ 3]   R[ 4]   R[ 5]   |
     *   \  R[ 6]   R[ 7]   R[ 8]   /
     *</pre>
     * If R.length == 16, the following matrix is returned:
     * <pre>
     *   /  R[ 0]   R[ 1]   R[ 2]   0  \
     *   |  R[ 4]   R[ 5]   R[ 6]   0  |
     *   |  R[ 8]   R[ 9]   R[10]   0  |
     *   \  0       0       0       1  /
     *</pre>
     *  @param rotationVector the rotation vector to convert
     *  @param R an array of floats in which to store the rotation matrix
     */
    public static void getRotationMatrixFromVector(float[] R, float[] rotationVector) {
	float q0 = (float)Math.sqrt(1 - rotationVector[0]*rotationVector[0] -
				    rotationVector[1]*rotationVector[1] -
				    rotationVector[2]*rotationVector[2]);
	float q1 = rotationVector[0];
	float q2 = rotationVector[1];
	float q3 = rotationVector[2];

	float sq_q1 = 2 * q1 * q1;
	float sq_q2 = 2 * q2 * q2;
	float sq_q3 = 2 * q3 * q3;
	float q1_q2 = 2 * q1 * q2;
	float q3_q0 = 2 * q3 * q0;
	float q1_q3 = 2 * q1 * q3;
	float q2_q0 = 2 * q2 * q0;
	float q2_q3 = 2 * q2 * q3;
	float q1_q0 = 2 * q1 * q0;

	if(R.length == 9) {
	    R[0] = 1 - sq_q2 - sq_q3;
	    R[1] = q1_q2 - q3_q0;
	    R[2] = q1_q3 + q2_q0;

	    R[3] = q1_q2 + q3_q0;
	    R[4] = 1 - sq_q1 - sq_q3;
	    R[5] = q2_q3 - q1_q0;

	    R[6] = q1_q3 - q2_q0;
	    R[7] = q2_q3 + q1_q0;
	    R[8] = 1 - sq_q1 - sq_q2;
	} else if (R.length == 16) {
	    R[0] = 1 - sq_q2 - sq_q3;
	    R[1] = q1_q2 - q3_q0;
	    R[2] = q1_q3 + q2_q0;
	    R[3] = 0.0f;

	    R[4] = q1_q2 + q3_q0;
	    R[5] = 1 - sq_q1 - sq_q3;
	    R[6] = q2_q3 - q1_q0;
	    R[7] = 0.0f;

	    R[8] = q1_q3 - q2_q0;
	    R[9] = q2_q3 + q1_q0;
	    R[10] = 1 - sq_q1 - sq_q2;
	    R[11] = 0.0f;

	    R[12] = R[13] = R[14] = 0.0f;
	    R[15] = 1.0f;
	}
    }

    /** Helper function to convert a rotation vector to a normalized quaternion.
     *  Given a rotation vector (presumably from a ROTATION_VECTOR sensor), returns a normalized
     *  quaternion in the array Q.  The quaternion is stored as [w, x, y, z]
     *  @param rv the rotation vector to convert
     *  @param Q an array of floats in which to store the computed quaternion
     */
    public static void getQuaternionFromVector(float[] Q, float[] rv) {
	float w = (float)Math.sqrt(1 - rv[0]*rv[0] - rv[1]*rv[1] - rv[2]*rv[2]);
	//In this case, the w component of the quaternion is known to be a positive number

	Q[0] = w;
	Q[1] = rv[0];
	Q[2] = rv[1];
	Q[3] = rv[2];
    }

#endif

void MapGraphicsScene::sensorReadingChanged()
{
	#ifdef Q_OS_ANDROID
	QtMobility::QSensor *sensor = qobject_cast<QtMobility::QSensor*>(sender());
	const QtMobility::QSensorReading *reading = sensor->reading();
	QString sensorName(sensor->objectName());
	if(sensorName == "QAccelerometer")
	{
		// Gravity removal code from http://developer.android.com/reference/android/hardware/SensorEvent.html#values

		// alpha is calculated as t / (t + dT)
		// with t, the low-pass filter's time-constant
		// and dT, the event delivery rate

		static double gravity[3] = {1,1,1};

		const float alpha = 0.8;

		double values[3] = {
		    reading->value(0).toDouble(),
		    reading->value(1).toDouble(),
		    reading->value(2).toDouble()
		};

		gravity[0] = alpha * gravity[0] + (1 - alpha) * values[0];
		gravity[1] = alpha * gravity[1] + (1 - alpha) * values[1];
		gravity[2] = alpha * gravity[2] + (1 - alpha) * values[2];

		double linear_acceleration[3] = { 0,0,0 };
		linear_acceleration[0] = values[0] - gravity[0];
		linear_acceleration[1] = values[1] - gravity[1];
		linear_acceleration[2] = values[2] - gravity[2];

		//qDebug() << " --> Accelerometer: " << linear_acceleration[0] << linear_acceleration[1] << linear_acceleration[2];
	}
	else
	if(sensorName == "QGyroscope")
	{
		static const float NS2S = 1.0f / 1000000000.0f;
		static float deltaRotationVector[4] = {0,0,0,0};
		static float timestamp = 0;

		double values[3] = {
		    reading->value(0).toDouble(),
		    reading->value(1).toDouble(),
		    reading->value(2).toDouble()
		};

		// This timestep's delta rotation to be multiplied by the current rotation
		// after computing it from the gyro sample data.
		if (timestamp != 0)
		{
			float dT = (reading->timestamp() - timestamp) * NS2S;

			// Axis of the rotation sample, not normalized yet.
			float axisX = values[0];
			float axisY = values[1];
			float axisZ = values[2];

			// Calculate the angular speed of the sample
			float omegaMagnitude = sqrt(axisX*axisX + axisY*axisY + axisZ*axisZ);

			// Normalize the rotation vector if it's big enough to get the axis
			if (omegaMagnitude > 1.0) { // 1.0 was EPSILON
				axisX /= omegaMagnitude;
				axisY /= omegaMagnitude;
				axisZ /= omegaMagnitude;
			}

			// Integrate around this axis with the angular speed by the timestep
			// in order to get a delta rotation from this sample over the timestep
			// We will convert this axis-angle representation of the delta rotation
			// into a quaternion before turning it into the rotation matrix.
			float thetaOverTwo = omegaMagnitude * dT / 2.0f;
			float sinThetaOverTwo = sin(thetaOverTwo);
			float cosThetaOverTwo = cos(thetaOverTwo);
			deltaRotationVector[0] = sinThetaOverTwo * axisX;
			deltaRotationVector[1] = sinThetaOverTwo * axisY;
			deltaRotationVector[2] = sinThetaOverTwo * axisZ;
			deltaRotationVector[3] = cosThetaOverTwo;
		}
		timestamp = reading->timestamp();
		//float deltaRotationMatrix[] = { 0,0,0, 0,0,0, 0,0,0 };
		// TODO: Translate getRotationMatrixFromVector() [above] from Java to C++
		//SensorManager.getRotationMatrixFromVector(deltaRotationMatrix, deltaRotationVector);
		// User code should concatenate the delta rotation we computed with the current rotation
		// in order to get the updated rotation.
		//float rotationCurrent[] = { 1,1,1, 1,1,1, 1,1,1 };
		//rotationCurrent = rotationCurrent * deltaRotationMatrix;


	}
	else
	{
		/*
		int count = reading->valueCount();
		qDebug() << "Receved "<< count <<" value readings from: " << sensor->objectName();
		for(int i=0; i<count; i++)
			qDebug() << "\t Value " << i << ":" << reading->value(i);
		*/
	}
	#endif
}

#ifdef Q_OS_ANDROID
/*
void MapGraphicsScene::positionUpdated(QtMobility::QGeoPositionInfo info)
{
	qDebug() << "Position updated:" << info;
}
*/
#endif

void MapGraphicsScene::triggerRender()
{
	if(m_renderTrigger.isActive())
		m_renderTrigger.stop();
	m_renderTrigger.start(0);
	m_mapWindow->setStatusMessage("Rendering signal map...");
}

void MapGraphicsScene::setMarkApMode(bool flag)
{
	m_markApMode = flag;
	if(flag)
		m_mapWindow->setStatusMessage("Touch and hold map to mark AP location");
	else
		m_mapWindow->flagApModeCleared(); // resets push button state as well
}

void MapGraphicsScene::clear()
{
	qDeleteAll(m_apInfo.values());
	m_apInfo.clear();
	m_sigValues.clear();
	QGraphicsScene::clear();
	
	
	m_bgFilename = "";
	//m_bgPixmap = QPixmap(4000,4000); //QApplication::desktop()->screenGeometry().size());
	m_bgPixmap = QPixmap(2000,2000); //QApplication::desktop()->screenGeometry().size());
	m_bgPixmap.fill(Qt::white);
	
	QPainter p(&m_bgPixmap);
	p.setPen(QPen(Qt::gray,3.0));
	int fontSize = 10;
	int pad = fontSize/2;
#ifdef Q_OS_ANDROID
	fontSize = 5;
	pad = fontSize;
#endif
	p.setFont(QFont("Monospace", fontSize, QFont::Bold));
	
	for(int x=0; x<m_bgPixmap.width(); x+=64)
	{
		p.drawLine(x,0,x,m_bgPixmap.height());
		for(int y=0; y<m_bgPixmap.height(); y+=64)
		{
			p.drawText(x+pad,y+fontSize+pad*3,QString("%1x%2").arg(x/64).arg(y/64));
			p.drawLine(0,y,m_bgPixmap.width(),y);
		}
	}
	
	m_bgPixmapItem = addPixmap(m_bgPixmap);
	m_bgPixmapItem->setZValue(0);
	
	addSigMapItem();
	
	#ifdef Q_OS_ANDROID
	QString size = "64x64";
	#else
	QString size = "32x32";
	#endif
	
	// This is just a dummy pixmap - m_userItem will receive a rendered item later 
	QPixmap pix(tr(":/data/images/%1/stock-media-rec.png").arg(size));
	m_userItem = addPixmap(pix);
	m_userItem->setOffset(-(pix.width()/2.),-(pix.height()/2.));
	m_userItem->setVisible(false);
	m_userItem->setZValue(150);
	//m_userItem->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
	
	m_longPressSpinner = new LongPressSpinner();
	m_longPressSpinner->setVisible(false);
	m_longPressSpinner->setZValue(999);
	m_longPressSpinner->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
	addItem(m_longPressSpinner);
	
	m_currentMapFilename = "";
}

void MapGraphicsScene::setBgFile(QString filename)
{
	m_bgFilename = filename;
	m_bgPixmap = QPixmap(m_bgFilename);
	
	m_bgPixmapItem->setPixmap(m_bgPixmap);
	m_bgPixmapItem->setTransformationMode(Qt::SmoothTransformation);
	m_bgPixmapItem->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
	
	QSize sz = m_bgPixmap.size();
	
	setSceneRect(-sz.width() * 2., -sz.height() * 2., sz.width()*4., sz.height()*4.);
}


/// tinted() and grayscaled() from graphics-dojo on git (Qt labs) - not my creation

// Convert an image to grayscale and return it as a new image
QImage grayscaled(const QImage &image)
{
    QImage img = image;
    int pixels = img.width() * img.height();
    unsigned int *data = (unsigned int *)img.bits();
    for (int i = 0; i < pixels; ++i) {
        int val = qGray(data[i]);
        data[i] = qRgba(val, val, val, qAlpha(data[i]));
    }
    return img;
}

// Tint an image with the specified color and return it as a new image
QImage tinted(const QImage &image, const QColor &color, QPainter::CompositionMode mode = QPainter::CompositionMode_Overlay)
{
    QImage resultImage(image.size(), QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&resultImage);
    painter.drawImage(0, 0, grayscaled(image));
    painter.setCompositionMode(mode);
    painter.fillRect(resultImage.rect(), color);
    painter.end();
    resultImage.setAlphaChannel(image.alphaChannel());
    return resultImage;
}


void MapGraphicsScene::addApMarker(QPointF point, QString mac)
{
	MapApInfo *info = apInfo(mac);
	info->point  = point;
	info->marked = true;
	
	QImage markerGroup(":/data/images/ap-marker.png");
	markerGroup = addDropShadow(markerGroup, (double)(markerGroup.width()/2));
		
	//markerGroup.save("apMarkerDebug.png");
	
	markerGroup = tinted(markerGroup, colorForSignal(1.0, mac));

	QGraphicsPixmapItem *item = addPixmap(QPixmap::fromImage(markerGroup));
	item->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
	
	double w2 = (double)(markerGroup.width())/2.;
	double h2 = (double)(markerGroup.height())/2.;
	item->setPos(point);
	item->setOffset(-w2,-h2);
	
// 	item->setFlag(QGraphicsItem::ItemIsMovable);
// 	item->setFlag(QGraphicsItem::ItemSendsGeometryChanges);
// 	item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	item->setZValue(99);
}


void MapGraphicsScene::addSigMapItem()
{
	
	m_sigMapItem = new SigMapItem();
	addItem(m_sigMapItem);
	
/*	QPixmap empty(m_bgPixmap.size());
	empty.fill(Qt::transparent);
	
	m_sigMapItem = addPixmap(empty); 
	m_sigMapItem->setTransformationMode(Qt::SmoothTransformation);
	m_sigMapItem->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
*/
	m_sigMapItem->setZValue(1);
	m_sigMapItem->setOpacity(0.75);
}

QLineF calcIntersect(QPointF p0, double r0, QPointF p1, double r1)
{ 
	// Based following code on http://paulbourke.net/geometry/2circle/
	// Test cases under "First calculate the distance d between the center of the circles. d = ||P1 - P0||."
	//QPointF delta = p1 - p0;
	//ouble d = sqrt(delta.x()*delta.x() + delta.y()*delta.y());
	double d = QLineF(p1,p0).length();
	
// 	qDebug() << "\t r0:"<<r0<<", r1:"<<r1<<", p0:"<<p0<<", p1:"<<p1<<", d:"<<d;
// 	qDebug() << "\t ratio0 * apMacToSignal[ap0]:"<<ratio0<<apMacToSignal[ap0];
// 	qDebug() << "\t ratio1 * apMacToSignal[ap1]:"<<ratio1<<apMacToSignal[ap1];
	
	if(d > r0 + r1)
	{
		// If d > r0 + r1 then there are no solutions, the circles are separate.
		qDebug() << "calcIntersect(): d > r0 + r1, there are no solutions, the circles are separate.";
		//m_userItem->setVisible(false);
		return QLineF();
	}
	
	if(d < fabs(r0 - r1))
	{
		// If d < |r0 - r1| then there are no solutions because one circle is contained within the other.
		qDebug() << "calcIntersect(): d < |r0 - r1|, there are no solutions because one circle is contained within the other.";
		//m_userItem->setVisible(false);
		return QLineF();
	}
	
	if(d == 0. && r0 == r1)
	{
		// If d = 0 and r0 = r1 then the circles are coincident and there are an infinite number of solutions.
		qDebug() << "calcIntersect(): d = 0 and r0 = r1, the circles are coincident and there are an infinite number of solutions.";
		//m_userItem->setVisible(false);
		return QLineF();
	}
	
// 	// a = (r0^2 - r1^2 + d^2 ) / (2 d)
// 	double a = ( (r0*r0) - (r1*r1) + (d*d) ) / (2 * d);
// 	
// 	// P2 = P0 + a ( P1 - P0 ) / d
// 	QPointF p2 = p0 +  a * (p1 - p0) / d;
// 	
// 	// x3 = x2 +- h ( y1 - y0 ) / d
// 	// y3 = y2 -+ h ( x1 - x0 ) / d
// 	double y3 = p2.y() + 

	/// From http://2000clicks.com/mathhelp/GeometryConicSectionCircleIntersection.aspx:
	
	// d^2 = (xB-xA)^2 + (yB-yA)^2 
	// K = (1/4)sqrt(((rA+rB)^2-d^2)(d^2-(rA-rB)^2))
	// x = (1/2)(xB+xA) + (1/2)(xB-xA)(rA^2-rB^2)/d^2 ± 2(yB-yA)K/d^2 
	// y = (1/2)(yB+yA) + (1/2)(yB-yA)(rA^2-rB^2)/d^2 ± -2(xB-xA)K/d^2
	
	/// K = (1/4)sqrt(((rA+rB)^2-d^2)(d^2-(rA-rB)^2))
	double K = .25 * sqrt( ((r0+r1)*(r0+r1) - d*d) * (d*d - (r0-r1) * (r0-r1)) );
	
	/// x = (1/2)(xB+xA) + (1/2)(xB-xA)(rA^2-rB^2)/d^2 ± 2(yB-yA)K/d^2
	double x1 = .5 * (p1.x()+p0.x()) + .5 * (p1.x()-p0.x()) * (r0*r0-r1*r1) / (d*d) + 2 * (p1.y()-p0.y()) * K / (d*d);
	double x2 = .5 * (p1.x()+p0.x()) + .5 * (p1.x()-p0.x()) * (r0*r0-r1*r1) / (d*d) - 2 * (p1.y()-p0.y()) * K / (d*d);
	
	/// y = (1/2)(yB+yA) + (1/2)(yB-yA)(rA^2-rB^2)/d^2 ± -2(xB-xA)K/d^2
	double y1 = .5 * (p1.y()+p0.y()) + .5 * (p1.y()-p0.y()) * (r0*r0-r1*r1) / (d*d) + 2 * (p1.x()-p0.x()) * K / (d*d);
	double y2 = .5 * (p1.x()+p0.y()) + .5 * (p1.y()-p0.y()) * (r0*r0-r1*r1) / (d*d) - 2 * (p1.x()-p0.x()) * K / (d*d);
	
	return QLineF(x1,y1,x2,y2);
}

double dist2(const QPointF &p0, const QPointF &p1)
{
	QPointF delta = p1 - p0;
	return delta.x()*delta.x() + delta.y()*delta.y();
}

double dist23(const QPointF &p0, const QPointF &p1, const QPointF &p2)
{
	QPointF delta = p2 - p1 - p0; 
	return delta.x()*delta.x() + delta.y()*delta.y();
}

// double operator-(const QPointF &a, const QPointF &b)
// {
// 	return sqrt(dist2(a,b));
// }

QPointF operator*(const QPointF&a, const QPointF& b) { return QPointF(a.x()*b.x(), a.y()*b.y()); }

void MapGraphicsScene::scanFinished(QList<WifiDataResult> results)
{
	
	QPointF realPoint;

// 	static bool firstScan = true;
// 	if(!firstScan)
// 		return;
// 		
// 	firstScan = false;
	
	//qDebug() << "MapGraphicsScene::scanFinished(): currentThreadId:"<<QThread::currentThreadId();
	
	// Sort the results high-low
	qSort(results.begin(), results.end(), MapGraphicsScene_sort_WifiDataResult);
	
	m_lastScanResults = results;
	update(); // allow HUD to update
	
	if(!m_showMyLocation || !m_autoGuessApLocations)
		return;
	
	if(m_sigValues.isEmpty())
		return;
	
		
	QSize origSize = m_bgPixmap.size();
	QImage image(origSize, QImage::Format_ARGB32_Premultiplied);
	QPointF offset = QPointF();
	
	memset(image.bits(), 0, image.byteCount());
	
	if(m_showMyLocation)
	{
		/// JUST for debugging
		// 	realPoint = m_sigValues.last()->point;
		// 	results   = m_sigValues.last()->scanResults;
		//
		
		
		QPointF userLocation  = QPointF(-1000,-1000);
	
		// Need at least two *known* APs registered on the map - with out that, we don't even need to check the results
		if(m_apInfo.values().size() < 2)
		{
			//qDebug() << "MapGraphicsScene::scanFinished(): Less than two APs marked, unable to guess user location";
			return;
		}
		
		// Build a hash table of MAC->Signal for eash access and a list of MACs in decending order of signal strength
		QStringList apsVisible;
		QHash<QString,double> apMacToSignal;
		QHash<QString,int> apMacToDbm;
		foreach(WifiDataResult result, results)
		{
			//qDebug() << "MapGraphicsScene::scanFinished(): Checking result: "<<result; 
			if(!apMacToSignal.contains(result.mac) &&
			   !apInfo(result.mac)->point.isNull())
			{
				apsVisible << result.mac;
				apMacToSignal.insert(result.mac, result.value);
	
				#ifdef OPENCV_ENABLED
				// Use Kalman to filter the dBm value
				MapApInfo *info = apInfo(result.mac);
				info->kalman.predictionUpdate(result.dbm, 0);
	
				float value = (float)result.dbm, tmp = 0;
				info->kalman.predictionReport(value, tmp);
				apMacToDbm.insert(result.mac, (int)value);
				result.dbm = (int)value;
				
				#else
	
				apMacToDbm.insert(result.mac, result.dbm);
				
				#endif
	
				qDebug() << "[apMacToSignal] mac:"<<result.mac<<", val:"<<result.value<<", dBm:"<<result.dbm;
			}
		}
		
		// Need at least two APs *visible* in results *AND* marked on the MAP
		// (apsVisible only contains APs marked on the map AND in the latest scan results) 
		if(apsVisible.size() < 2)
		{
			//qDebug() << "MapGraphicsScene::scanFinished(): Less than two known APs marked AND visble, unable to guess user location";
			return;
		}
	/*	
		// For each AP, average the ratio of pixels-to-signalLevel by averaging the location of every reading for that AP in relation to the marked location of the AP.
		// This is used to calculate the radius of the APs coverage.
		QHash<QString,int>    apRatioCount;
		QHash<QString,double> apRatioSums;
		QHash<QString,double> apRatioAvgs;
		
		
		foreach(QString apMac, apsVisible)
		{
			QPointF center = apInfo(apMac)->point;
			
			double maxDistFromCenter = -1;
			double maxSigValue = 0.0;
			foreach(SigMapValue *val, m_sigValues)
			{
				if(val->hasAp(apMac))
				{
					double dx = val->point.x() - center.x();
					double dy = val->point.y() - center.y();
					double distFromCenter = sqrt(dx*dx + dy*dy);
					if(distFromCenter > maxDistFromCenter)
					{
						maxDistFromCenter = distFromCenter;
						maxSigValue = val->signalForAp(apMac);
					}
				}
			}
			
			apRatioAvgs[apMac] = maxDistFromCenter;// / maxSigValue;
			qDebug() << "[ratio calc] "<<apMac<<": maxDistFromCenter:"<<maxDistFromCenter<<", macSigValue:"<<maxSigValue<<", ratio:"<<apRatioAvgs[apMac]; 
		}
				
		
	// 	foreach(SigMapValue *val, m_sigValues)
	// 	{
	// 		//qDebug() << "MapGraphicsScene::scanFinished(): [ratio calc] val:"<<val; 
	// 		
	// 		// Remember, apsVisible has all the APs *visible* in this scan result *AND* marked in the map 
	// 		foreach(QString apMac, apsVisible)
	// 		{
	// 			//qDebug() << "MapGraphicsScene::scanFinished(): [ratio calc] apMac:"<<apMac;
	// 			if(val->hasAp(apMac) &&
	// 			  !apInfo(apMac)->point.isNull()) // TODO Since apsVisible are only APs marked, do we need to test this still?
	// 			{
	// 				QPointF delta = apInfo(apMac)->point - val->point;
	// 				double d = sqrt(delta.x()*delta.x() + delta.y()*delta.y());
	// 				//double dist = d;
	// 				d = d / val->signalForAp(apMac);
	// 				
	// 				//qDebug() << "MapGraphicsScene::scanFinished(): [ratio calc] \t ap:"<<apMac<<", d:"<<d<<", val:"<<val->signalForAp(apMac)<<", dist:"<<dist; 
	// 				
	// 				// Incrememnt counters/sums
	// 				if(!apRatioSums.contains(apMac))
	// 				{
	// 					apRatioSums.insert(apMac, d);
	// 					apRatioCount.insert(apMac, 1);
	// 				}
	// 				else
	// 				{
	// 					apRatioSums[apMac] += d;
	// 					apRatioCount[apMac] ++;
	// 				}
	// 			}
	// 		}
	// 	}
	// 	
	// 	// Average the values found above to determine the pixel-signal ratio
	// 	foreach(QString key, apRatioSums.keys())
	// 	{
	// 		apRatioAvgs[key] = apRatioSums[key] / apRatioCount[key];
	// 		//qDebug() << "MapGraphicsScene::scanFinished(): [ratio calc] final avg for mac:"<<key<<", avg:"<<apRatioAvgs[key]<<", count:"<<apRatioCount[key]<<", sum:"<<apRatioSums[key];
	// 	}
		
		
		// apsVisible implicitly is sorted strong->weak signal
		QString ap0 = apsVisible[0];
		QString ap1 = apsVisible[1];
		QString ap2 = apsVisible.size() > 2 ? apsVisible[2] : "";
		
		//qDebug() << "MapGraphicsScene::scanFinished(): ap0: "<<ap0<<", ap1:"<<ap1; 
		
		double ratio0 = 0;
		double ratio1 = 0;
		double ratio2 = 0;
		
		if(apRatioAvgs.isEmpty())
		{
			qDebug() << "MapGraphicsScene::scanFinished(): Need at least one reading for ANY marked AP to establish even a 'best guess' ratio";
			return;
		}
		else
		{
			// It's possible two APs are VISIBLE and MARKED, *but* they don't have a signal reading for one of the APs, so we guess by using the ratio of the first AP in the apRatioAvgs hash 
			if(!apRatioAvgs.contains(ap0))
			{
				ratio0 = apRatioAvgs[apRatioAvgs.keys().first()];
				qDebug() << "MapGraphicsScene::scanFinished(): Need at least one reading for "<<ap0<<" to establish correct dBm to pixel ratio, cheating by using first";
			}
			else
				ratio0 = apRatioAvgs[ap0];
				
			if(!apRatioAvgs.contains(ap1))
			{
				ratio1 = apRatioAvgs[apRatioAvgs.keys().first()];
				qDebug() << "MapGraphicsScene::scanFinished(): Need at least one reading for "<<ap1<<" to establish correct dBm to pixel ratio, cheating by using first";
			}
			else
				ratio1 = apRatioAvgs[ap1];
				
			if(!apRatioAvgs.contains(ap2))
			{
				ratio2 = apRatioAvgs[apRatioAvgs.keys().first()];
				qDebug() << "MapGraphicsScene::scanFinished(): Need at least one reading for "<<ap2<<" to establish correct dBm to pixel ratio, cheating by using first";
			}
			else
				ratio2 = apRatioAvgs[ap2];
		}
		
		
		// this is safe to not check apInfo() for null ptr, since apInfo() auto-creates a MapApInfo object for any previously-unknown macs
		QPointF p0 = apInfo(ap0)->point;
		QPointF p1 = apInfo(ap1)->point;
		QPointF p2 = !ap2.isEmpty() ? apInfo(ap2)->point : QPointF();
		//qDebug() << "MapGraphicsScene::scanFinished(): p0: "<<p0<<", p1:"<<p1;
		
		double r0 = apMacToSignal[ap0] * ratio0;
		double r1 = apMacToSignal[ap1] * ratio1;
		double r2 = apMacToSignal[ap2] * ratio2;
		
		qDebug() << "[radius] "<<ap0<<": r0:"<<r0<<", ratio0:"<<ratio0<<", signal:"<<apMacToSignal[ap0];
		qDebug() << "[radius] "<<ap1<<": r1:"<<r1<<", ratio1:"<<ratio1<<", signal:"<<apMacToSignal[ap1];
		qDebug() << "[radius] "<<ap2<<": r2:"<<r2<<", ratio2:"<<ratio2<<", signal:"<<apMacToSignal[ap2];
		
		QLineF line  = calcIntersect(p0,r0, p1,r1);
		QLineF line2 = calcIntersect(p1,r1, p2,r2);
		QLineF line3 = calcIntersect(p2,r2, p0,r0);
		
		// Make an image to contain x1,y1 and x2,y2, or at least the user's icon
		QRectF itemWorldRect = QRectF(line.p1(), QSizeF(
			line.p1().x() - line.p2().x(), 
			line.p1().y() - line.p2().y()))
				.normalized();
		
		//qDebug() << "MapGraphicsScene::scanFinished(): Calculated (x1,y1):"<<line.p1()<<", (x2,y2):"<<line.p2()<<", itemWorldRect:"<<itemWorldRect<<", center:"<<itemWorldRect.center();
		//qDebug() << "MapGraphicsScene::scanFinished(): #2 debug: "<<ap2<<": "<<p2<<", radius: "<<r2;
		
		// Attempt http://stackoverflow.com/questions/9747227/2d-trilateration
		
		// Define:
		// ex,x = (P2x - P1x) / sqrt((P2x - P1x)2 + (P2y - P1y)2)
		// ex,y = (P2y - P1y) / sqrt((P2x - P1x)2 + (P2y - P1y)2)
	
		// Steps:
		// 1. ex = (P2 - P1) / ‖||P2 - P1||
		QPointF vex = (p1 - p0) / sqrt(dist2(p1, p0));
		// 2. i = ex(P3 - P1)
		QPointF vi  = vex * (p2 - p1);
		// ey = (P3 - P1 - i · ex) / ||‖P3 - P1 - i · ex||
		QPointF ey = (p2 - p1 - (vi * vex)) / dist23(p2,p0,vi*vex);
		// d = ||‖P2 - P1‖||
		double d = sqrt(dist2(p1,p0));
		// j = ey(P3 - P1)
		QPointF vj = vex * (p1 - p0);
		// x = (r12 - r22 + d2) / 2d
		double x = (r0*r0 - r1*r1 + d*d)/(2*d);(void)x;//ignore warning
		// TODO: This line doesn't make sense: (type conversion?)
		// y = (r12 - r32 + i2 + j2) / 2j - ix / j
		//double y = (r0*r0 - r2*r2 + vi*vi + vj*vj) / (2*vj - vix/vij);
		
		
		// That didn't work, so let's tri triangulation
		
		
		
	// 	  C
	// 	  *
	// 	  |`.
	// 	 b|  `. a
	// 	  |    `.
	// 	  |      `.
	// 	A *--------* B
	// 	      c
			
		
		QLineF apLine(p1,p0);
		
		QLineF realLine(p0,realPoint);
		double realAngle = realLine.angle();
		
		QLineF realLine2(p1,realPoint);
		double realAngle2 = realLine2.angle();
		
	// 	double la = r0; //realLine2.length(); //r0;
	// 	double lb = r1; //realLine.length(); //r1;
		double lc = apLine.length(); //sqrt(dist2(p1,p0));
		
		
	// 	double fA = 1./(10.*n);
	// 	double fB = pTx - pRx + gTx + gRx;
	// 	double fC = -Xa + (20.*log10(m)) - (20.*log10(4.*Pi));
	// 	
		
		QPointF lossFactor2 = deriveObservedLossFactor(ap2);	
		QPointF lossFactor1 = deriveObservedLossFactor(ap1);
		QPointF lossFactor0 = deriveObservedLossFactor(ap0);
		
		// Store newly-derived loss factors into apInfo() for use in dBmToDistance()
		apInfo(ap2)->lossFactor = lossFactor2;
		apInfo(ap1)->lossFactor = lossFactor1;
		apInfo(ap0)->lossFactor = lossFactor0;
		
	// 	qDebug() << "[formula] codified comparrison (ap1): "<<dBmToDistance(apMacToDbm[ap1], ap1);
	// 	qDebug() << "[formula] codified comparrison (ap0): "<<dBmToDistance(apMacToDbm[ap0], ap0);
		
		double lb = dBmToDistance(apMacToDbm[ap1], ap1) * m_meterPx;
		double la = dBmToDistance(apMacToDbm[ap0], ap0) * m_meterPx;
		
		r2 = dBmToDistance(apMacToDbm[ap2], ap2) * m_meterPx;
		
		qDebug() << "[dump1] "<<la<<lb<<lc;
		qDebug() << "[dump2] "<<realAngle<<realAngle2<<apLine.angle();
		qDebug() << "[dump3] "<<p0<<p1<<realPoint;
		qDebug() << "[dump4] "<<realLine.angleTo(apLine)<<realLine2.angleTo(apLine)<<realLine2.angleTo(realLine);
			
	// 	la= 8;
	// 	lb= 6;
	// 	lc= 7;
		
	// 	la = 180;
	// 	lb = 238;
	// 	lc = 340;
		
		// cos A = (b2 + c2 - a2)/2bc
		// cos B = (a2 + c2 - b2)/2ac
		// cos C = (a2 + b2 - c2)/2ab
		double cosA = (lb*lb + lc*lc - la*la)/(2*lb*lc);
		double cosB = (la*la + lc*lc - lb*lb)/(2*la*lc);
		double cosC = (la*la + lb*lb - lc*lc)/(2*la*lb);
		double angA = acos(cosA)* 180.0 / Pi;
		double angB = acos(cosB)* 180.0 / Pi;
		double angC = acos(cosC)* 180.0 / Pi;
		//double angC = 180 - angB - angA;
				
		
		QLineF userLine(p1,QPointF());
		QLineF userLine2(p0,QPointF());
		
		//double userLineAngle = 90+45-angB+apLine.angle()-180;
		//userLineAngle *= -1;
		//userLine.setAngle(angA + apLine.angle() + 90*3);
		userLine.setAngle(angA + apLine.angle());
	// 	userLine.setLength(r1);
		userLine.setLength(lb);
		
		userLine2.setAngle(angA + angC + apLine.angle());
	// 	userLine2.setLength(r0);
		userLine2.setLength(la);
		
		double realAngle3 = realLine2.angleTo(realLine);
		
		double errA = realAngle  - angA;
		double errB = realAngle2 - angB;
		double errC = realAngle3 - angC;
		
		qDebug() << "Triangulation: ang(A-C):"<<angA<<angB<<angC<<", cos(A-C):"<<cosA<<cosB<<cosC;
		qDebug() << "Triangulation: err(A-C):"<<errA<<errB<<errC;
		qDebug() << "Triangulation: abs(A-C):"<<realAngle<<realAngle2<<apLine.angle();
		qDebug() << "Triangulation: realAngle:"<<realAngle<<"/"<<realLine.length()<<", realAngle2:"<<realAngle2<<"/"<<realLine2.length();
		qDebug() << "Triangulation: apLine.angle:"<<apLine.angle()<<", line1->line2 angle:"<<userLine.angleTo(userLine2)<<", realAngle3:"<<realAngle3;
		
		//QPointF calcPoint = userLine.p2();
		
		QPointF calcPoint = triangulate(ap0, apMacToDbm[ap0], 
						ap1, apMacToDbm[ap1]);
		
		userLine  = QLineF(p1, calcPoint);
		userLine2 = QLineF(p0, calcPoint);
		
	*/	
		
	/*
		QImage image(
			(int)qMax((double)itemWorldRect.size().width(),  64.) + 10,
			(int)qMax((double)itemWorldRect.size().height(), 64.) + 10,
			QImage::Format_ARGB32_Premultiplied);
	*/
		QRect picRect = m_sigMapItem->picture().boundingRect();
//		/*QPointF */offset = picRect.topLeft();
			
	// 	QImage image(picRect.size(), QImage::Format_ARGB32_Premultiplied);
	// 	QPainter p(&image);
	// 	p.translate(-offset);
		
// 		QSize origSize = m_bgPixmap.size();
// 		QImage image(origSize, QImage::Format_ARGB32_Premultiplied);
//		offset = QPointF();
		
//		memset(image.bits(), 0, image.byteCount());
		QPainter p(&image);
		
		//QPainter p(&image);
		
		//p.setPen(Qt::black);
		
	//	QRectF rect(QPointF(0,0),itemWorldRect.size());
	// 	QRectF rect = itemWorldRect;
	// 	QPointF center = rect.center();
		
		#ifdef Q_OS_ANDROID
		QString size = "64x64";
		#else
		QString size = "32x32";
		#endif
		
		double penWidth = 20.0;
		
		QHash<QString,bool> drawnFlag;
		QHash<QString,int> badLossFactor;
		
		QVector<QPointF> userPoly;
		
		QPointF avgPoint(0.,0.);
		int count = 0;
		
		int numAps = apsVisible.size();
		for(int i=0; i<numAps; i++)
		{
			QString ap0 = apsVisible[i];
			QString ap1 = (i < numAps - 1) ? apsVisible[i+1] : apsVisible[0];
			
			QPointF p0 = apInfo(ap0)->point;
			QPointF p1 = apInfo(ap1)->point;
			
			QPointF calcPoint = triangulate(ap0, apMacToDbm[ap0],
							ap1, apMacToDbm[ap1]);
			
			if(isnan(calcPoint.x()) || isnan(calcPoint.y()))
			{
				if(!badLossFactor.contains(ap0))
					badLossFactor[ap0] = 0;
				else
					badLossFactor[ap0] = badLossFactor[ap0] + 1;
				
				if(!badLossFactor.contains(ap1))
					badLossFactor[ap1] = 0;
				else
					badLossFactor[ap1] = badLossFactor[ap1] + 1;
					
				qDebug() << "\t NaN: "<<badLossFactor[ap0]<<" / "<<badLossFactor[ap1];
			}
			else
			{
				avgPoint += calcPoint;
				count ++;
			}
		}
		
	// 	avgPoint.setX( avgPoint.x() / count );
	// 	avgPoint.setY( avgPoint.y() / count );
		avgPoint /= count;
		
		foreach(QString ap, apsVisible)
		{
			if(badLossFactor[ap] > 0)
			{
				MapApInfo *info = apInfo(ap);
				
				double avgMeterDist = QLineF(avgPoint, info->point).length() / m_meterPx;
				double absLossFactor = deriveLossFactor(ap, apMacToDbm[ap], avgMeterDist/*, gRx*/);
	
				if(isnan(absLossFactor))
				{
					qDebug() << "MapGraphicsScene::scanFinished(): "<<ap<<": Unable to correct, received NaN loss factor, avgMeterDist:"<<avgMeterDist<<", absLossFactor:"<<absLossFactor;
				}
				else
				{
					QPointF lossFactor = info->lossFactor;
					if(apMacToDbm[ap] > info->shortCutoff)
						lossFactor.setY(absLossFactor);
					else
						lossFactor.setX(absLossFactor);
	
					info->lossFactor = lossFactor;
	
					qDebug() << "MapGraphicsScene::scanFinished(): "<<ap<<": Corrected loss factor:" <<lossFactor<<", avgMeterDist:"<<avgMeterDist<<", absLossFactor:"<<absLossFactor;
				}
			}
		}
	
		QPointF avgPoint2(0.,0.);
		count = 0;
	
	// 	int numAps = apsVisible.size();
		for(int i=0; i<numAps; i++)
		{
	// 		QString ap0 = apsVisible[i];
	// 		QString ap1 = (i < numAps - 1) ? apsVisible[i+1] : apsVisible[0];
	
			QString ap0 = apsVisible[0];
			QString ap1 = apsVisible[i]; //(i < numAps - 1) ? apsVisible[i+1] : apsVisible[0];
			
			QPointF p0 = apInfo(ap0)->point;
			QPointF p1 = apInfo(ap1)->point;
			
			QPointF calcPoint = triangulate(ap0, apMacToDbm[ap0],
							ap1, apMacToDbm[ap1]);
			
			// We assume triangulate() already stored drived loss factor into apInfo()
			double r0 = dBmToDistance(apMacToDbm[ap0], ap0) * m_meterPx;
			double r1 = dBmToDistance(apMacToDbm[ap1], ap1) * m_meterPx;
			
			qDebug() << "MapGraphicsScene::scanFinished(): "<<ap0<<"->"<<ap1<<": Point:" <<calcPoint<<", r0:"<<r0<<", r1:"<<r1<<", p0:"<<p0<<", p1:"<<p1;
			
			if(isnan(calcPoint.x()) || isnan(calcPoint.y()))
				qDebug() << "MapGraphicsScene::scanFinished(): "<<ap0<<"->"<<ap1<<": - Can't render ellipse - calcPoint is NaN";
			else
			{
				userPoly << calcPoint;
				
				QColor color0 = baseColorForAp(ap0);
				if(!drawnFlag.contains(ap0))
				{
					drawnFlag.insert(ap0, true);
					
					p.setPen(QPen(color0, penWidth));
					
					if(isnan(r0))
						qDebug() << "MapGraphicsScene::scanFinished(): "<<ap0<<"->"<<ap1<<": - Can't render ellipse p0/r0 - radius 0 is NaN";
					else
						p.drawEllipse(p0, r0, r0);
				}
				
				QColor color1 = baseColorForAp(ap1);
		
				if(!drawnFlag.contains(ap1))
				{
					drawnFlag.insert(ap1, true);
					
					p.setPen(QPen(color1, penWidth));
					
					if(isnan(r1))
						qDebug() << "MapGraphicsScene::scanFinished(): "<<ap0<<"->"<<ap1<<": - Can't render ellipse p0/r0 - radius 1 is NaN";
					else
						p.drawEllipse(p1, r1, r1);
					
				}
				
				p.setPen(QPen(color0, penWidth));
				p.drawLine(p0, calcPoint);
				
				p.setPen(QPen(color1, penWidth));
				p.drawLine(p1, calcPoint);
				
				avgPoint2 += calcPoint;
				count ++;
			}
			
			//p.setPen(QPen(Qt::gray, penWidth));
			//p.drawLine(p0, p1);
			
			//break;
		}
		
	// 	avgPoint2.setX( avgPoint.x() / count );
	// 	avgPoint2.setY( avgPoint.y() / count );
		avgPoint2 /= count;
	
		
		p.setPen(QPen(Qt::red, 30.));
		//p.drawPolygon(userPoly);
	
		penWidth = 100;
		
		p.setPen(QPen(Qt::green, 10.));
		p.setBrush(QColor(0,0,0,127));
		if(!isnan(avgPoint.x()) && !isnan(avgPoint.y()))
			p.drawEllipse(avgPoint, penWidth, penWidth);
		
		if(!isnan(avgPoint2.x()) && !isnan(avgPoint2.y()))
		{
			if(avgPoint == avgPoint2)
				p.setPen(QPen(Qt::yellow, 10.));
			else
				p.setPen(QPen(Qt::red, 10.));
				
			p.setBrush(QColor(0,0,0,127));
			p.drawEllipse(avgPoint2, penWidth, penWidth);
			
			#ifdef OPENCV_ENABLED
			m_kalman.predictionUpdate((float)avgPoint2.x(), (float)avgPoint2.y());
			
			float x = avgPoint2.x(), y = avgPoint2.y();
			m_kalman.predictionReport(x, y);
			QPointF thisPredict(x,y);
			
			p.setPen(QPen(Qt::blue, 15.));
				
			p.setBrush(QColor(0,0,0,127));
			p.drawEllipse(thisPredict, penWidth, penWidth);
			#endif
		}
		
	/*	
		p.setPen(QPen(Qt::blue,penWidth));
	// 	QPointF s1 = line.p1() - itemWorldRect.topLeft();
	// 	QPointF s2 = line.p2() - itemWorldRect.topLeft();
		QPointF s1 = line.p1();
		QPointF s2 = line.p2();
		
		//p.drawLine(s1,s2);
		
		QImage user(tr(":/data/images/%1/stock-media-rec.png").arg(size));
		p.drawImage((s1+s2)/2., user.scaled(128,128));
		
		p.setBrush(Qt::blue);
	// 	p.drawRect(QRectF(s1.x()+3,s1.y()+3,6,6));
	// 	p.drawRect(QRectF(s2.x()-3,s2.y()-3,6,6));
		
		p.setBrush(QBrush());
		//p.drawEllipse(center + QPointF(5.,5.), center.x(), center.y());
		p.setPen(QPen(Qt::white,penWidth));
	// 	p.drawEllipse(p0, r0,r0);
		p.drawEllipse(p0, la,la);
		p.setPen(QPen(Qt::red,penWidth));
	// 	p.drawEllipse(p1, r1,r1);
		p.drawEllipse(p1, lb,lb);
		p.setPen(QPen(Qt::green,penWidth));
		if(!p2.isNull())
			p.drawEllipse(p2, r2,r2);
		
		p.setPen(QPen(Qt::white,penWidth));
		p.drawRect(QRectF(p0-QPointF(5,5), QSizeF(10,10)));
		p.setPen(QPen(Qt::red,penWidth));
		p.drawRect(QRectF(p1-QPointF(5,5), QSizeF(10,10)));
		p.setPen(QPen(Qt::green,penWidth));
		if(!p2.isNull())
			p.drawRect(QRectF(p2-QPointF(5,5), QSizeF(10,10)));
		
	// 	p.setPen(QPen(Qt::red,penWidth));
	// 	p.drawLine(line);
	// 	p.drawLine(line2);
	// 	p.drawLine(line3);
		
		penWidth *=2;
		
		p.setPen(QPen(Qt::white,penWidth));
		p.drawLine(realLine);
		p.setPen(QPen(Qt::gray,penWidth));
		p.drawLine(realLine2);
	
		penWidth /=2;
		p.setPen(QPen(Qt::darkGreen,penWidth));
		p.drawLine(apLine);
		p.setPen(QPen(Qt::darkYellow,penWidth));
		p.drawLine(userLine);
		p.setPen(QPen(Qt::darkRed,penWidth));
		p.drawLine(userLine2);
	
	// 	p.setPen(QPen(Qt::darkYellow,penWidth));
	// 	p.drawRect(QRectF(calcPoint-QPointF(5,5), QSizeF(10,10)));
	// 	p.setPen(QPen(Qt::darkGreen,penWidth));
	// 	p.drawRect(QRectF(realPoint-QPointF(5,5), QSizeF(10,10)));
		
	*/
		p.end();
	}

	if(m_autoGuessApLocations)
	{
		/// JUST for debugging
		// 	realPoint = m_sigValues.last()->point;
		// 	results   = m_sigValues.last()->scanResults;
		//

		// Build a hash table of MAC->Signal for eash access and a list of MACs in decending order of signal strength
		QStringList apsVisible;
		QHash<QString,double> apMacToSignal;
		QHash<QString,int> apMacToDbm;
		
		foreach(WifiDataResult result, results)
		{
			//qDebug() << "MapGraphicsScene::scanFinished(): Checking result: "<<result;
			if(!apMacToSignal.contains(result.mac))
			{
				apsVisible << result.mac;
				apMacToSignal.insert(result.mac, result.value);

				#ifdef OPENCV_ENABLED
				// Use Kalman to filter the dBm value
				MapApInfo *info = apInfo(result.mac);
				info->kalman.predictionUpdate(result.dbm, 0);

				float value = (float)result.dbm, tmp = 0;
				info->kalman.predictionReport(value, tmp);
				apMacToDbm.insert(result.mac, (int)value);
				result.dbm = (int)value;

				#else

				apMacToDbm.insert(result.mac, result.dbm);

				#endif

				qDebug() << "[apLocationGuess:apMacToSignal] mac:"<<result.mac<<", val:"<<result.value<<", dBm:"<<result.dbm;
			}
		}


		if(m_sigValues.isEmpty())
		{
			qDebug() << "[apLocationGuess] No readings, can't guess anything";
		}


		QPainter p(&image);


		#ifdef Q_OS_ANDROID
		QString size = "64x64";
		#else
		QString size = "32x32";
		#endif

		double penWidth = 20.0;

		QHash<QString,bool> drawnFlag;
		QHash<QString,int> badLossFactor;

		QVector<QPointF> userPoly;

		QPointF avgPoint(0.,0.);
		int count = 0;


		QHash<QString, QList<SigMapValue*> > valuesByAp;
		foreach(QString apMac, apsVisible)
		{
			// Build a list of values for this AP
			valuesByAp.insert(apMac, QList<SigMapValue *>());
			foreach(SigMapValue *val, m_sigValues)
				if(val->hasAp(apMac))
					valuesByAp[apMac].append(val);

			// Sort values by signal
			QList<SigMapValue *> list = valuesByAp[apMac];
			qSort(list.begin(), list.end(), SigMapRenderer_sort_SigMapValue_bySignal);
			valuesByAp[apMac] = list;


			MapApInfo *info = apInfo(apMac);
			
			qDebug() << "[apLocationGuess] Found "<<list.size()<<" readings for apMac:"<<apMac<<", ESSID:" << info->essid;

			if(list.isEmpty())
				continue;
			
			QPointF avgPoint2(0.,0.);
			count = 0;
			int numVals = list.size();

		// 	int numAps = apsVisible.size();
			SigMapValue *val0 = list.first();
			
			for(int i=1; i<numVals; i++)
			{
				SigMapValue *val1 = list[i];
				
				QPointF calcPoint = triangulateAp(apMac, val0, val1);

				// We assume triangulate() already stored drived loss factor into apInfo()
				double r0 = dBmToDistance(val0->signalForAp(apMac, true), apMac) * m_meterPx;
				double r1 = dBmToDistance(val1->signalForAp(apMac, true), apMac) * m_meterPx;

				//qDebug() << "MapGraphicsScene::scanFinished(): "<<ap0<<"->"<<ap1<<": Point:" <<calcPoint<<", r0:"<<r0<<", r1:"<<r1<<", p0:"<<p0<<", p1:"<<p1;

				if(isnan(calcPoint.x()) || isnan(calcPoint.y()))
					qDebug() << "MapGraphicsScene::scanFinished(): "<<ap0<<"->"<<ap1<<": - Can't render ellipse - calcPoint is NaN";
				else
				{
					//userPoly << calcPoint;

					QColor color0 = baseColorForAp(apMac);

					p.setPen(QPen(color0, penWidth));
					p.drawLine(p0, calcPoint);

					avgPoint2 += calcPoint;
					count ++;
				}

				//break;
			}

			avgPoint2 /= count;

			p.setPen(QPen(Qt::red, 30.));
			//p.drawPolygon(userPoly);

			penWidth = 100;

			p.setPen(QPen(Qt::green, 10.));
			p.setBrush(QColor(0,0,0,127));
			if(!isnan(avgPoint.x()) && !isnan(avgPoint.y()))
				p.drawEllipse(avgPoint, penWidth, penWidth);

			if(!isnan(avgPoint2.x()) && !isnan(avgPoint2.y()))
			{
// 				if(avgPoint == avgPoint2)
// 					p.setPen(QPen(Qt::yellow, 10.));
// 				else
					p.setPen(QPen(Qt::green, 10.));

				p.setBrush(QColor(0,0,0,127));
				p.drawEllipse(avgPoint2, penWidth, penWidth);

				#ifdef OPENCV_ENABLED
				info->locationGuessKalman.predictionUpdate((float)avgPoint2.x(), (float)avgPoint2.y());

				float x = avgPoint2.x(), y = avgPoint2.y();
				info->locationGuessKalman.predictionReport(x, y);
				QPointF thisPredict(x,y);

				p.setPen(QPen(Qt::darkGreen, 15.));

				p.setBrush(QColor(0,0,0,127));
				p.drawEllipse(thisPredict, penWidth, penWidth);

				info->locationGuess = thisPredict;

				#else

				info->locationGuess = avgPoint2;
				
				#endif
				
			}

		}

		p.end();
	}

	m_userItem->setPixmap(QPixmap::fromImage(image));
	//m_userItem->setOffset(-(((double)image.width())/2.), -(((double)image.height())/2.));
	//m_userItem->setPos(itemWorldRect.center());
	m_userItem->setOffset(offset);
	m_userItem->setPos(0,0);
	
// 	m_userItem->setOffset(0,0); //-(image.width()/2), -(image.height()/2));
// 	m_userItem->setPos(0,0); //itemWorldRect.center());
	
	m_userItem->setVisible(true);

}

QPointF MapGraphicsScene::triangulate(QString ap0, int dBm0, QString ap1, int dBm1)
{
	MapApInfo *inf0 = apInfo(ap0);
	MapApInfo *inf1 = apInfo(ap1);
	
	if(inf0->lossFactor.isNull() ||
	  (inf0->lossFactorKey > -1 && inf0->lossFactorKey != m_sigValues.size()))
	{
		QPointF lossFactor0 = deriveObservedLossFactor(ap0);
		
		// Store newly-derived loss factors into apInfo() for use in dBmToDistance()
		inf0->lossFactor    = lossFactor0;
		inf0->lossFactorKey = m_sigValues.size();
	}
		
	if(inf1->lossFactor.isNull() ||
	  (inf1->lossFactorKey > -1 && inf1->lossFactorKey != m_sigValues.size()))
	{
		QPointF lossFactor1 = deriveObservedLossFactor(ap1);
		
		// Store newly-derived loss factors into apInfo() for use in dBmToDistance()
		inf1->lossFactor    = lossFactor1;
		inf1->lossFactorKey = m_sigValues.size();
	}
	
	// Get location of APs (in pixels, obviously)
	QPointF p0 = inf0->point;
	QPointF p1 = inf1->point;
	
	// Get a line from one AP to the other (need this for length and angle)
	QLineF apLine(p1,p0);
	
	double lc = apLine.length();
	double la = dBmToDistance(dBm0, ap0) * m_meterPx; // dBmToDistance returns meters, convert to pixels
	double lb = dBmToDistance(dBm1, ap1) * m_meterPx;
	
// 	qDebug() << "[dump1] "<<la<<lb<<lc;
// 	qDebug() << "[dump2] "<<realAngle<<realAngle2<<apLine.angle();
// 	qDebug() << "[dump3] "<<p0<<p1<<realPoint;
// 	qDebug() << "[dump4] "<<realLine.angleTo(apLine)<<realLine2.angleTo(apLine)<<realLine2.angleTo(realLine);
	
	/*
	
	  C
	  *
	  |`.
	 b|  `. a
	  |    `.
	  |      `.
	A *--------* B
	      c
	
	*/
	
	// Calculate the angle (A)
	double cosA = (lb*lb + lc*lc - la*la)/(2*lb*lc);
	double angA = acos(cosA) * 180.0 / Pi;
	
	// Create a line from the appros AP and set it's angle to the calculated angle
	QLineF userLine(p1,QPointF());
	userLine.setAngle(angA + apLine.angle());
	userLine.setLength(lb);
	
// 	qDebug() << "MapGraphicsScene::triangulate("<<ap0<<","<<dBm0<<","<<ap1<<","<<dBm1<<"): ang(A):"<<angA<<", cos(A):"<<cosA;
// 	qDebug() << "MapGraphicsScene::triangulate("<<ap0<<","<<dBm0<<","<<ap1<<","<<dBm1<<"): apLine.angle:"<<apLine.angle();
// 	qDebug() << "MapGraphicsScene::triangulate("<<ap0<<","<<dBm0<<","<<ap1<<","<<dBm1<<"): la:"<<la<<", lb:"<<lb<<", lc:"<<lc;
	
	
	// Return the end-point of the line we just created - that's the point where 
	// the signals of the two APs intersect (supposedly, any error in the result is largely from dBmToDistance(), and, by extension, deriveObservedLossFactor())
	return userLine.p2();
}

double MapGraphicsScene::deriveLossFactor(QString apMac, int pRx, double distMeters, double gRx)
{
	const double m   =  0.12; // (meters) - wavelength of 2442 MHz, freq of 802.11b/g radio
	const double Xa  =  3.00; // (double)rand()/(double)RAND_MAX * 17. + 3.; // normal rand var, std dev a=[3,20]
	
	MapApInfo *info = apInfo(apMac);
	double pTx = info->txPower;
	double gTx = info->txGain;
	
	double logDist       = log10(distMeters);
	double adjustedPower = (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi));
	double n             = 1./(logDist / adjustedPower)/10.;
	
	return n;
}

QPointF MapGraphicsScene::deriveObservedLossFactor(QString apMac)
{
	const double m   =  0.12; // (meters) - wavelength of 2442 MHz, freq of 802.11b/g radio
	const double Xa  =  3.00; // (double)rand()/(double)RAND_MAX * 17. + 3.; // normal rand var, std dev a=[3,20]
	
	MapApInfo *info = apInfo(apMac);
	double pTx = info->txPower;
	double gTx = info->txGain;
	
	int shortCutoff = info->shortCutoff;
	
	QPointF lossFactor(2.,2.);
	
	int method = 1;
	if(method == 1)
	{
		// Use a reworked distance formula to calculate lossFactor for each point, then average together
			
		double shortFactor = 0.,
		       longFactor  = 0.;
		
		int    shortCount  = 0,
		       longCount   = 0;
		
		foreach(SigMapValue *val, m_sigValues)
		{
			if(val->hasAp(apMac))
			{
				double pRx = val->signalForAp(apMac, true); // true = return raw dBM
				double gRx = val->rxGain;
				
				// Assuming: logDist = (1/(10*n)) * (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi));
				// Rearranging gives:
				// logDist = (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi)) / (10n)
				// =
				// 1/10n = logDist / (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi))
				// =
				// n = 1/(logDist / (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi)))/10
				//
				// Paraphrased: 
				// adjustedPower = (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi))
				// logDist = 1/10n * adjustedPower 
				// 	(or simply: logDist = adjustedPower/(10n))
				// n = 1/(logDist/adjustedPower)/10
				// Where logDist = log(distFromAp * metersPerPixel)
				// NOTE: Any log above is base 10 as in log10(), above, not natrual logs as in log()
				
				double logDist       = log10(QLineF(info->point,val->point).length() / m_meterPx);
				double adjustedPower = (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi));
				double n             = 1./(logDist / adjustedPower)/10.;
				
				if(pRx > shortCutoff)
				{
					shortFactor += n;
					shortCount  ++;
				}
				else
				{
					longFactor  += n;
					longCount   ++;
				}
			}
		}
		
		if(!shortCount)
		{
			shortFactor = 2.;
			shortCount  = 1;
		}
		
		if(!longCount)
		{
			longFactor = 1.;
			longCount  = 1;
		}
		
		lossFactor = QPointF( longFactor /  longCount,
				     shortFactor / shortCount);
	
		// TODO calc MSE for the resultant N as error += ( (dBmToDistance(dBm, lossFactor)-realDist)^2 ) foreach reading; error/= readings
	
	}
	else
	if(method == 2)
	{
		// Try to minimize MSE using a gradient decent search for n
		// Stop when MSE < 0.1 OR num iterations > 1_000_000 OR (zig-zag TODO)
		
		const double errorCutoff = 0.01; // minimum eror
		const int maxIterations = 1000000;
		
		double error = 1.0; // starting error, meaningless
		int numIterations = 0;
		
		double shortFactor = 6.0; // starting factor guesses
		double longFactor  = 2.0; // starting factor guesses
		
		double stepFactor = 0.001; // minimum step 
		double stepChangeFactor = 0.0001; // amount to change step by 
		double shortStep = -0.5; // start with large steps, decrease as we get closer
		double longStep  = -0.5; // start with large steps, decrease as we get closer
		
		int shortCutoff = info->shortCutoff; // typically -49 dBm, ghreater than this, pRx is assumed less than 15 meters to AP, less than -49, assumed further than 15m from AP
		
		// Values to prevent zig-zagging around local minima
		double lastError = 0.0;
		double lastError1 = 0.0;
		bool zigZag = false;
		
		while(error > errorCutoff && numIterations < maxIterations && !zigZag)
		{
			double shortErrorSum = 0.;
			int shortErrorCount  = 0;
			
			double longErrorSum  = 0.;
			int longErrorCount   = 0;
			
			foreach(SigMapValue *val, m_sigValues)
			{
				if(val->hasAp(apMac))
				{
					double pRx = val->signalForAp(apMac, true); // true = return raw dBM
					double gRx = val->rxGain;
					double realDist = QLineF(info->point,val->point).length() / m_meterPx;
				
					double n = pRx < shortCutoff ? longFactor : shortFactor;
					
					double logDist = (1/(10*n)) * (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi));
					double invLog  = pow(10, logDist); // distance in meters
					
					double err = realDist - invLog; //pow(invLog - realDist, 2);
					
					if(pRx < shortCutoff)
					{
						longErrorSum   += err;
						longErrorCount ++;
					}
					else
					{
						shortErrorSum   += err;
						shortErrorCount ++;
					}
				}
			}
			
			double shortErrAvg = shortErrorSum / shortErrorCount;
			double longErrAvg  =  longErrorSum /  longErrorCount;
			error = (fabs(shortErrAvg) + fabs(longErrAvg)) / 2; // used for termination
			
			if(lastError1 == error)
				zigZag = true;
			
			lastError1 = lastError;
			lastError = error;
			
			shortStep = ((shortErrAvg > 0) ? -1 : 1) * fabs(shortStep) * stepChangeFactor;
			longStep  = ((longErrAvg  > 0) ? -1 : 1) * fabs(longStep)  * stepChangeFactor;
			
			int ss = shortStep < 0 ? -1:1;
			int ls = longStep  < 0 ? -1:1;
			shortStep = qMax(stepFactor, fabs(shortStep)) * ss;
			longStep  = qMax(stepFactor, fabs(longStep))  * ls;
			
			shortFactor += shortStep;
			longFactor  += longStep;
			
			qDebug() << "MapGraphicsScene::deriveObservedLossFactor(): "<<numIterations<<":"<<error<<" (z:"<<zigZag<<"): shortErr:"<<shortErrAvg<<", longErr:"<<longErrAvg<<", shortFactor:"<<shortFactor<<", longFactor:"<<longFactor<<", shortStep:"<<shortStep<<", longStep:"<<longStep;
			
			numIterations ++;
		}

		lossFactor = QPointF( longFactor, shortFactor );
	}

	//qDebug() << "MapGraphicsScene::deriveObservedLossFactor(): "<<apMac<<": Derived: "<<lossFactor;
	//qDebug() << "MapGraphicsScene::deriveObservedLossFactor(): "<<apMac<<": Debug: pTx:"<<pTx<<", gTx:"<<gTx<<" shortCutoff:"<<shortCutoff;
	
	return lossFactor;
}

QPointF MapGraphicsScene::deriveImpliedLossFactor(QString apMac)
{
	const double m   =  0.12; // (meters) - wavelength of 2442 MHz, freq of 802.11b/g radio
	const double Xa  =  3.00; // (double)rand()/(double)RAND_MAX * 17. + 3.; // normal rand var, std dev a=[3,20]

	MapApInfo *info = apInfo(apMac);
	double pTx = info->txPower;
	double gTx = info->txGain;

	int shortCutoff = info->shortCutoff;

	QPointF lossFactor(2.,2.);

	// Use a reworked distance formula to calculate lossFactor for each point, then average together

	double shortFactor = 0.,
		longFactor  = 0.;

	int    shortCount  = 0,
		longCount   = 0;

	foreach(SigMapValue *val, m_sigValues)
	{
		if(val->hasAp(apMac))
		{
			double pRx = val->signalForAp(apMac, true); // true = return raw dBM
			double gRx = val->rxGain;

			// TODO Rework formula and this algorithm to account for the distance between two points instead of point and AP

			// Assuming: logDist = (1/(10*n)) * (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi));
			// Rearranging gives:
			// logDist = (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi)) / (10n)
			// =
			// 1/10n = logDist / (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi))
			// =
			// n = 1/(logDist / (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi)))/10
			//
			// Paraphrased:
			// adjustedPower = (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi))
			// logDist = 1/10n * adjustedPower
			// 	(or simply: logDist = adjustedPower/(10n))
			// n = 1/(logDist/adjustedPower)/10
			// Where logDist = log(distFromAp * metersPerPixel)
			// NOTE: Any log above is base 10 as in log10(), above, not natrual logs as in log()

			double logDist       = log10(QLineF(info->point,val->point).length() / m_meterPx);
			double adjustedPower = (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi));
			double n             = 1. / (logDist / adjustedPower) / 10.;

			if(pRx > shortCutoff)
			{
				shortFactor += n;
				shortCount  ++;
			}
			else
			{
				longFactor  += n;
				longCount   ++;
			}
		}
	}

	if(!shortCount)
	{
		shortFactor = 2.;
		shortCount  = 1;
	}

	if(!longCount)
	{
		longFactor = 1.;
		longCount  = 1;
	}

	lossFactor = QPointF( longFactor /  longCount,
				shortFactor / shortCount);

	// TODO calc MSE for the resultant N as error += ( (dBmToDistance(dBm, lossFactor)-realDist)^2 ) foreach reading; error/= readings
	return lossFactor;
}

		
double MapGraphicsScene::dBmToDistance(int dBm, QString apMac, double gRx)
{
	MapApInfo *info = apInfo(apMac);
	QPointF n  = info->lossFactor;
	int cutoff = info->shortCutoff;
	double pTx = info->txPower;
	double gTx = info->txGain;
	return dBmToDistance(dBm, n, cutoff, pTx, gTx, gRx);
}

double MapGraphicsScene::dBmToDistance(int dBm, QPointF lossFactor, int shortCutoff, double txPower, double txGain,  double rxGain)
{
// 	double n   = /* 1.60*/0; /// TUNE THIS 
// 	n = apMacToDbm[ap1] < -49 ? 1.6 : 2.0;
// 	double m   =  0.12; // (meters) - wavelength of 2442 MHz, freq of 802.11b/g radio
// 	double Xa  =  3.00; // (double)rand()/(double)RAND_MAX * 17. + 3.; // normal rand var, std dev a=[3,20]
// 	double pTx = 11.80; // (dBm) - Transmit power, [1] est stock WRT54GL at 19 mW, [1]=http://forums.speedguide.net/showthread.php?253953-Tomato-and-Linksys-WRT54GL-transmit-power
// 	double pRx = apMacToDbm[ap1]; /// Our measurement
// 	double gTx =  5.00; // (dBi) - Gain of stock WRT54GL antennas
// 	double gRx =  3.00; // (dBi) - Estimated gain of laptop antenna, neg dBi. Internal atennas for phones/laptops range from (-3,+3)
	
	// NOTE Formula used below is from from http://web.mysites.ntu.edu.sg/aschfoh/public/Shared%20Documents/pub/04449717-icics07.pdf
	// double logDist = (1/(10*n)) * (pTx - pRx + gTx + gRx - Xa + 20*log(m) - 20*log(4*Pi));
	
	// Possibly integrate method of solving loss factor as described in http://continuouswave.com/whaler/reference/pathLoss.html 
	
	// Alternative forumla given at http://www.moxa.com/newsletter/connection/2008/03/Figure_out_transmission_distance_from_wireless_device_specs.htm#04
	// Merging the two provided gives:
	// r=pow(10,(pTx+(gTx+gRx)*n-pRx)/20) / (41.88 * f) where r is in km and f is the frequency, e.g. 2442
	
	
	// n is the loss factor describing obstacles, walls, reflection, etc, can be derived from existing data with driveObservedLossFactor()
	double n   = dBm < shortCutoff ? lossFactor.x() : lossFactor.y();
	
	double m   =  0.12; // (meters) - wavelength of 2442 MHz, freq of 802.11b/g radio
	double Xa  =  3.00; // (double)rand()/(double)RAND_MAX * 17. + 3.; // normal rand var, std dev a=[3,20]
	
	double logDist = (1/(10*n)) * (txPower - dBm + txGain + rxGain - Xa + 20*log10(m) - 20*log10(4*Pi));
	double distMeters = pow(10, logDist); // distance in meters
	
	//qDebug() << "MapGraphicsScene::dBmToDistance(): "<<dBm<<": meters:"<<distMeters<<", Debug: n:"<<n<<", txPower:"<<txPower<<", txGain:"<<txGain<<", rxGain:"<<rxGain;
	
	return distMeters;
}
	
	
void MapGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent * mouseEvent)
{
	if(m_longPressTimer.isActive())
	{
		m_longPressTimer.stop();
		m_longPressCountTimer.stop();
	}
	m_longPressTimer.start();
	m_pressPnt = mouseEvent->lastScenePos();
	
	m_longPressSpinner->setPos(m_pressPnt);
	
	m_longPressCountTimer.start();
	m_longPressCount = 0;
	
	QGraphicsScene::mousePressEvent(mouseEvent);
}

void MapGraphicsScene::invalidateLongPress(QPointF curPoint)
{
	if(m_longPressTimer.isActive())
	{
		if(!curPoint.isNull())
		{
			#ifdef Q_OS_ANDROID
			const double maxDist = 16.;
			#else
			const double maxDist = 1.;
			#endif
			
			// Don't invalidate longpress if curPoint (current mouse/finger position)
			// is within 'maxDist' radius of the starting location
			if(QLineF(m_pressPnt, curPoint).length() < maxDist)
				return;
		}
		
		m_longPressTimer.stop();
		m_longPressCountTimer.stop();
		m_longPressSpinner->setVisible(false);
		
		//m_mapWindow->setStatusMessage("");
	}
}

void MapGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent * mouseEvent)
{
	invalidateLongPress();
		
	QGraphicsScene::mouseReleaseEvent(mouseEvent);
}

void MapGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent * mouseEvent)
{
	invalidateLongPress(mouseEvent->lastScenePos());
		
	QGraphicsScene::mouseMoveEvent(mouseEvent);
}

void MapGraphicsScene::longPressCount()
{
	m_longPressCount ++;
	int maxCount = m_longPressTimer.interval() / m_longPressCountTimer.interval();
	double part  = ((double)m_longPressCount) / ((double)maxCount);
	
	m_longPressSpinner->setProgress(part);
	
	int percent = (int)(part * 100.); 
	
	if(percent > 100.) // we missed a cancel call somehwere...
	{
		m_longPressCountTimer.stop();
	}
	else
	{
		m_mapWindow->setStatusMessage(tr("Waiting %1%").arg(percent), m_longPressTimer.interval());
	}
}

void MapGraphicsScene::longPressTimeout()
{
	m_longPressSpinner->setGoodPress(true);

	if(m_markApMode)
	{
		/// Scan for APs nearby and prompt user to choose AP
		
		m_mapWindow->setStatusMessage(tr("<font color='green'>Scanning...</font>"));
		
		QList<WifiDataResult> results = m_scanIf.scanResults();
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
					//#ifndef Q_OS_ANDROID
					triggerRender();
					//#endif
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
		
		QList<WifiDataResult> results = m_scanIf.scanResults();
		m_mapWindow->setStatusMessage(tr("<font color='green'>Scan finished!</font>"), 3000);
		
		if(results.size() > 0)
		{
			// call addSignalMarker() with presspnt
			addSignalMarker(m_pressPnt, results);
			
			m_mapWindow->setStatusMessage(tr("Added marker for %1 APs").arg(results.size()), 3000);
			
			// Render map overlay
			//#ifndef Q_OS_ANDROID
			triggerRender();
			//#endif
			//renderSigMap();
			
			QStringList notFound;
			foreach(WifiDataResult result, results)
				if(!apInfo(result)->marked)
					notFound << QString("<font color='%1'>%2</font> (<i>%3</i>)")
						.arg(colorForSignal(result.value, result.mac).name())
						.arg(result.mac.right(6))
						.arg(result.essid); // last two octets of mac should be enough
					
			if(notFound.size() > 0)
			{
				m_mapWindow->setStatusMessage(tr("Ok, but %2 APs need marked: %1").arg(notFound.join(", ")).arg(notFound.size()), 10000);
			}
		}
		else
		{
			m_mapWindow->setStatusMessage(tr("<font color='red'>No APs found nearby</font>"), 3000);
		}
	}
}

void MapGraphicsScene::setRenderMode(RenderMode r)
{
	m_colorListForMac.clear(); // color gradient style change based on render mode
	m_renderMode = r;
	triggerRender();
}

/// ImageFilters class, inlined here for ease of implementation

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

/// End ImageFilters implementation


SigMapValue *MapGraphicsScene::addSignalMarker(QPointF point, QList<WifiDataResult> results)
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
		double iconX = iconSizeHalf/2.5 * numResults * cos(rads);
		double iconY = iconSizeHalf/2.5 * numResults * sin(rads);
		QRectF iconRect = QRectF(iconX - iconSizeHalf, iconY - iconSizeHalf, (double)iconSize, (double)iconSize);
		iconRects << iconRect;
		boundingRect |= iconRect;
	}
	
	boundingRect.adjust(-1,-1,+2,+2);
	
	//qDebug() << "MapGraphicsScene::addSignalMarker(): boundingRect:"<<boundingRect;
	
	QImage markerGroup(boundingRect.size().toSize(), QImage::Format_ARGB32_Premultiplied);
	memset(markerGroup.bits(), 0, markerGroup.byteCount());//markerGroup.fill(Qt::green);
	QPainter p(&markerGroup);
	
#ifdef Q_OS_ANDROID
	QFont font("", 6, QFont::Bold);
	QFont smallFont("", 4, QFont::Bold);
#else
	QFont font("", (int)(iconSize*.33), QFont::Bold);
	QFont smallFont("", (int)(iconSize*.20));
#endif

	p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);

	
	double zeroAdjX = boundingRect.x() < 0. ? fabs(boundingRect.x()) : 0.0; 
	double zeroAdjY = boundingRect.y() < 0. ? fabs(boundingRect.y()) : 0.0;
	
	for(int resultIdx=0; resultIdx<numResults; resultIdx++)
	{
		WifiDataResult result = results[resultIdx];
		
		QColor centerColor = colorForSignal(result.value, result.mac);
		
		QRectF iconRect = iconRects[resultIdx];
		iconRect.translate(zeroAdjX,zeroAdjY);
		
		//qDebug() << "MapGraphicsScene::addSignalMarker(): resultIdx:"<<resultIdx<<": iconRect:"<<iconRect<<", centerColor:"<<centerColor;
		
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
			//QString sigString = QString("%1%").arg((int)(result.value * 100));
			QString sigString = QString("%1d").arg((int)(result.dbm));
			
			// Calculate text location centered in icon
			p.setFont(font);
	
			QRect textRect = p.boundingRect(0, 0, INT_MAX, INT_MAX, Qt::AlignLeft, sigString);
			int textX = (int)(iconRect.width()/2  - textRect.width()/2  + iconSize*.1); // .1 is just a cosmetic adjustment to center it better
			#ifdef Q_OS_ANDROID
			int textY = (int)(iconRect.height()/2 - textRect.height()/2 + font.pointSizeF() + 16);
			#else
			int textY = (int)(iconRect.height()/2 - textRect.height()/2 + font.pointSizeF() * 1.33);
			#endif
			
			qDrawTextO(p,textX,textY,sigString);
			
			// Render AP ESSID
			QString essid = result.essid;
			
			// Calculate text location centered in icon
			p.setFont(smallFont);
			
			textRect = p.boundingRect(0, 0, INT_MAX, INT_MAX, Qt::AlignLeft, essid);
			textX = (int)(iconRect.width()/2  - textRect.width()/2  + font.pointSizeF()*.1); // .1 is just a cosmetic adjustment to center it better
			#ifdef Q_OS_ANDROID
			textY += font.pointSizeF() * 2;
			#else
			textY += (int)(font.pointSizeF()*0.95);
			#endif
			
			qDrawTextO(p,textX,textY,essid);
			
			
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
	
	// Create value record and graphics item
	SigMapValue *val = new SigMapValue(point, results);
	SigMapValueMarker *item = new SigMapValueMarker(val, QPixmap::fromImage(markerGroup));
	
	// Add to graphics scene
	addItem(item);
	
	// Place item->pos() at the center instead of top-left
	double w2 = (double)(markerGroup.width())/2.;
	double h2 = (double)(markerGroup.height())/2.;
	item->setPos(point);
	item->setOffset(-w2,-h2);
	
	// Possible future use:
	//item->setFlag(QGraphicsItem::ItemIsMovable);
	
	// Optimize modes and tweak flags 
	item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
 	item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	item->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
	item->setZValue(99);
	
	//item->setOpacity(0);
	
	// Add pointer to the item in the scene to the signal value for turning on/off per user
	val->marker = item;
	
	// Add to list of readings
	m_sigValues << val;
	
	return val;
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

/// SigMapValue method implementations:

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

/// End SigMapValue impl



SigMapValue *MapGraphicsScene::findNearest(QPointF match, QString apMac)
{
	if(match.isNull())
		return 0;
		
	SigMapValue *nearest = 0;
	double minDist = (double)INT_MAX;
	
	foreach(SigMapValue *val, m_sigValues)
	{
		if(val->consumed || !val->hasAp(apMac))
			continue;
			
		double dx = val->point.x() - match.x();
		double dy = val->point.y() - match.y(); 
		double dist = dx*dx + dy*dy; // dont need true dist, so dont sqrt
		if(dist < minDist)
		{
			nearest = val;
			minDist = dist;
		}
	}
	
	if(nearest)
		nearest->consumed = true;
	
	return nearest;
	
}


double MapGraphicsScene::getRenderLevel(double level,double angle,QPointF realPoint,QString apMac,QPointF center,double circleRadius)
{	
	foreach(SigMapValue *val, m_sigValues)
	{
		if(val->hasAp(apMac))
		{
		
			// First, conver the location of this SigMapValue to an angle and "level" (0-100%) in relation to the center
			double testLevel;
			double testAngle;
			if(val->renderDataDirty ||
			   val->renderCircleRadius != circleRadius)
			{
				QPointF calcPoint = val->point - center;
			
				testLevel = sqrt(calcPoint.x() * calcPoint.x() + calcPoint.y() * calcPoint.y());
				testAngle = atan2(calcPoint.y(), calcPoint.x());// * 180 / Pi;
			
				// Convert the angle from radians in the range of [-pi,pi] to degrees
				testAngle = testAngle/Pi2 * 360;
				if(testAngle < 0) // compensate for invalid Quad IV angles in relation to points returned by sin(rads(angle)),cos(rads(angle))
					testAngle += 360;
			
				// Conver the level to a 0-100% value
				testLevel = (testLevel / circleRadius) * 100.;
				
				val->renderCircleRadius = circleRadius;
				val->renderLevel = testLevel;
				val->renderAngle = testAngle; 
			}
			else
			{
				testLevel = val->renderLevel;
				testAngle = val->renderAngle;
			}
			
			// Get the level of signal read for the current AP at this locaiton
			double signalLevel = val->signalForAp(apMac) * 100.;
			
			// Get the difference in angle for this reading from the current angle,
			// and the difference in "level" of this reading from the current level.
			// Because:
			// Only values within X degrees and Y% of current point
			// will affect this point
			double angleDiff = fabs(testAngle - angle);
			double levelDiff = fabs(testLevel - level);
			
			// Now, calculate the straight-line distance of this reading to the current point we're rendering
			QPointF calcPoint = val->point - realPoint;
			double lineDist = sqrt(calcPoint.x() * calcPoint.x() + calcPoint.y() * calcPoint.y());
			//const double lineWeight = 200.; // NOTE arbitrary magic number
			double lineWeight = (double)m_renderOpts.radialLineWeight;
			
			// Calculate the value to adjust the render level by.
			double signalLevelDiff = (testLevel - signalLevel)    // Adjustment is based on the differences in the current render level vs the signal level read at this point 
						* (levelDiff / 100.)	      // Apply a weight based on the difference in level (the location its read at vs the level we're rendering - points further away dont affect as much)
						* (1./(lineDist/lineWeight)); // Apply a weight based on the straight-line difference from this location to the signal level
						
			// The whole theory we're working with here is that the rendering algorithm will go in a circle around the center,
			// starting at level "0", with the idea that the "level" is the "ideal signal level" - incrementing outward with
			// the level being expressed as a % of the circle radius. The rendering algo makes one complete circle at each 
			// level, calculating the ideal point on the circle for each point at that level.
			
			// To express the signal readings, at each point on the circle on the current level, we calculate a "render level", wherein we adjust the current "level"
			// (e.g. the radius, or distance for that point from the circle's center) based on the formula above.
			
			// The forumla above is designed to take into account the signal reading closest to the current point, moving the point closer to the circle's center or further away
			// if the signal reading is stronger than "it should be" at that level or weaker"
			
			// By "stronger (or weaker) than it should be", we mean signal reading at X% from the circles center reads Y%, where ||X-Y|| > 0
			
			// To show the effect of the readings, we allow the signal markers to "warp" the circle - readings stronger than they should be closer to the circle
			// will "pull in" the appros level (a 50% reading at 10% from the circle's center will "pull in" the "50% line")
			
			// Now, inorder to extrapolate the effect of the reading and interpolate it across multiple points in the circle, and to factor in the warping
			// effect of nearby readings (e.g. to bend the lineds toward those values, should they deviate from their ideal readings on the circle),
			// we apply several weights, based on the difference in distance from the circle's center the reading is vs the current level,
			// as well as the straight-line distance from the reading in question to the current "ideal" rendering point.
			
			// Then we sum "adjustment value" (signalLevelDiff) changes onto the level value, so that oposing readings can cancel out each other appropriatly.
			
			// The result is a radial flow diagram, warped to approximately fit the signal readings, and appears to properly model apparent real-world signal propogations.
			
			// This rendering model can be tuned/tweaked in a number of locations:
			//	- The "angleDiff" and "levelDiff" comparrison values, below - These specifiy the maximum allowable difference in 
			//	  angle that the current SigMapValue and max allowable diff level that the SigMapValue can be inorder for it to have any influence whatosever on the current level
			//	- The lineDist divisor above in the third parenthetical part of the signalLevelDiff equation - the value should be *roughly* what the max
			//	  straight-line dist is that can be expected by allowing the given angleDiff/levelDiff (below) - though the value is inverted (1/x) in order to smooth
			//	  the effects of this particular value on the overall adjustment
			//	Additionally, rendering quality and speed can be affected in the SigMapRenderer::render() function by adjusting:
			//	- "int steps" variable (number of circular sections to render)
			//	- "levelInc" variable - the size of steps from 0-100% to take (currently, it's set to .25, making 400 total steps from 0-100 - more steps, more detailed rendering
			//	- You can also tweak the line width algorithm right before the call to p.drawLine, as well as the painter opacity to suit tastes
					
			//qDebug() << "getRenderLevel: val "<<val->point<<", signal for "<<apMac<<": "<<(int)signalLevel<<"%, angleDiff:"<<angleDiff<<", levelDiff:"<<levelDiff<<", lineDist:"<<lineDist<<", signalLevelDiff:"<<signalLevelDiff;
			
			/*	
			if(angleDiff < 135. && // 1/3rd of 360*
			   levelDiff <  33.)   // 1/3rd of 100 levels
			*/
			if(angleDiff < m_renderOpts.radialAngleDiff &&
			   levelDiff < m_renderOpts.radialLevelDiff)
			{
				//double oldLev = level;
				level += signalLevelDiff;
				
				//qDebug() << "\t - level:"<<level<<" (was:"<<oldLev<<"), signalLevelDiff:"<<signalLevelDiff; 
				
				//qDebug() << "getRenderLevel: "<<center<<" -> "<<realPoint.toPoint()<<": \t " << (round(angle) - round(testAngle)) << " \t : "<<level<<" @ "<<angle<<", calc: "<<testLevel<<" @ "<<testAngle <<", "<<calcPoint;
			}
		}
	}	
	
	if(level < 0)
		level = 0;
		
	return level;
}

SigMapRenderer::SigMapRenderer(MapGraphicsScene* gs)
	// Cannot set gs as QObject() parent because then we can't move it to a QThread
	: QObject(), m_gs(gs)
{
	qRegisterMetaType<QPicture>("QPicture");
}

#define fuzzyIsEqual(a,b) ( fabs(a - b) < 0.00001 )
//#define fuzzyIsEqual(a,b) ( ((int) a) == ((int) b) )


QColor colorForValue(double v)
{
	int hue = qMax(0, qMin(359, (int)((1-v) * 359)));
	return QColor::fromHsv(hue, 255, 255);
}

class qPointValue {
public:
	qPointValue(QPointF p = QPointF()) : point(p), value(0.0), valueIsNull(true) {}
	qPointValue(QPointF p, double v)   : point(p), value(v),   valueIsNull(false) {}
	
	QPointF point;
	double  value;
	bool    valueIsNull;
	
	void setValue(double v)
	{
		value       = v;
		valueIsNull = false;
	}
	
	bool isNull()  { return  point.isNull() ||  valueIsNull; }
	bool isValid() { return !point.isNull() && !valueIsNull; }
};

// The following cubic interpolation functions are from http://www.paulinternet.nl/?page=bicubic
double cubicInterpolate (double p[4], double x) {
	return p[1] + 0.5 * x*(p[2] - p[0] + x*(2.0*p[0] - 5.0*p[1] + 4.0*p[2] - p[3] + x*(3.0*(p[1] - p[2]) + p[3] - p[0])));
}

double bicubicInterpolate (double p[4][4], double x, double y) {
	double arr[4];
	arr[0] = cubicInterpolate(p[0], y);
	arr[1] = cubicInterpolate(p[1], y);
	arr[2] = cubicInterpolate(p[2], y);
	arr[3] = cubicInterpolate(p[3], y);
	return cubicInterpolate(arr, x);
}

double tricubicInterpolate (double p[4][4][4], double x, double y, double z) {
	double arr[4];
	arr[0] = bicubicInterpolate(p[0], y, z);
	arr[1] = bicubicInterpolate(p[1], y, z);
	arr[2] = bicubicInterpolate(p[2], y, z);
	arr[3] = bicubicInterpolate(p[3], y, z);
	return cubicInterpolate(arr, x);
}

double nCubicInterpolate (int n, double* p, double coordinates[]) {
	//assert(n > 0);
	if(n <= 0)
		return -1;
	if (n == 1) {
		return cubicInterpolate(p, *coordinates);
	}
	else {
		double arr[4];
		int skip = 1 << (n - 1) * 2;
		arr[0] = nCubicInterpolate(n - 1, p, coordinates + 1);
		arr[1] = nCubicInterpolate(n - 1, p + skip, coordinates + 1);
		arr[2] = nCubicInterpolate(n - 1, p + 2*skip, coordinates + 1);
		arr[3] = nCubicInterpolate(n - 1, p + 3*skip, coordinates + 1);
		return cubicInterpolate(arr, *coordinates);
	}
}

//bool newQuad = false;
double quadInterpolate(QList<qPointValue> corners, double x, double y)
{
	double w = corners[2].point.x() - corners[0].point.x();
	double h = corners[2].point.y() - corners[0].point.y();
	
	bool useBicubic = true;
	if(useBicubic)
	{
		// Use bicubic impl from http://www.paulinternet.nl/?page=bicubic
		double unitX = (x - corners[0].point.x()) / w;
		double unitY = (y - corners[0].point.y()) / h;
		
		double	c1 = corners[0].value,
			c2 = corners[1].value,
			c3 = corners[2].value,
			c4 = corners[3].value;

		double p[4][4] = 
		{
// 			{c1,c1,c2,c2},
// 			{c1,c1,c2,c2},
// 			{c4,c4,c3,c3},
// 			{c4,c4,c3,c3}
			
			// Note: X runs DOWN, values below are (x1,y1)->(x1,y4), NOT (x1,y1)->(x4,y1) as above (commented out) 
			{c1,c1,c4,c4},
			{c1,c1,c4,c4},
			{c2,c2,c3,c3},
			{c2,c2,c3,c3}
		};
		
		double p2 = bicubicInterpolate(p, unitX, unitY);
		//if(p2 > 1.0 || unitX > 1. || unitY > 1. || newQuad)
		//if(newQuad && unitX > 0.99)
		//{
			//qDebug() << "bicubicInterpolate: unit:"<<unitX<<","<<unitY<<": p2:"<<p2<<", w:"<<w<<", h:"<<h<<", x:"<<x<<", y:"<<y<<", corners:"<<corners[0].point<<corners[1].point<<corners[2].point<<corners[3].point<<", v:"<<c1<<c2<<c3<<c4;
			
			//newQuad = false;
		//}
		
		return p2;
	}
	else
	{
		// Very simple implementation of http://en.wikipedia.org/wiki/Bilinear_interpolation
		double fr1 = (corners[2].point.x() - x) / w * corners[0].value
			   + (x - corners[0].point.x()) / w * corners[1].value;
	
		double fr2 = (corners[2].point.x() - x) / w * corners[3].value
			   + (x - corners[0].point.x()) / w * corners[2].value;
	
		double p1  = (corners[2].point.y() - y) / h * fr1
			   + (y - corners[0].point.y()) / h * fr2;
	
		//qDebug() << "quadInterpolate: "<<x<<" x "<<y<<": "<<p<<" (fr1:"<<fr1<<",fr2:"<<fr2<<",w:"<<w<<",h:"<<h<<")";
		return p1;
	}
}

bool qPointValue_sort_point(qPointValue a, qPointValue b)
{
	return  fuzzyIsEqual(a.point.y(), (int)b.point.y()) ?
		(int)a.point.x()  < (int)b.point.x() :
		(int)a.point.y()  < (int)b.point.y();
}

bool hasPoint(QList<qPointValue> list, QPointF point)
{
	foreach(qPointValue v, list)
		if(v.point == point)
			return true;
	return false;
}

qPointValue getPoint(QList<qPointValue> list, QPointF point)
{
	foreach(qPointValue v, list)
		if(v.point == point)
			return v;
	return qPointValue();
}

QList<qPointValue> testLine(QList<qPointValue> inputs, QList<qPointValue> existingPoints, QRectF bounds, QPointF p1, QPointF p2, int dir)
{
	QLineF boundLine(p1,p2);
	QList<qPointValue> outputs;
	
	qPointValue c1, c2;
	if(!hasPoint(existingPoints, boundLine.p1()) &&
	   !hasPoint(outputs,        boundLine.p1()))
	{
		c1 = qPointValue(boundLine.p1(), 0.); // NOTE assumingunknown corners have value of 0.
		outputs << c1;
	}
	else
	{
		c1 = getPoint(inputs, boundLine.p1());
	}
	
	if(!hasPoint(existingPoints, boundLine.p2()) &&
	   !hasPoint(outputs,        boundLine.p2()))
	{
		c2 = qPointValue(boundLine.p2(), 0.); // NOTE assuming unknown corners have value of 0.;
		outputs << c2;
	}
	else
	{
		c2 = getPoint(inputs, boundLine.p2());
	}
	
	if(dir == 1 || dir == 3)
	{
		if(c1.point.x() > c2.point.x())
		{
			qPointValue tmp = c1;
			c1 = c2;
			c2 = tmp;
		}
	}
	else
	{
		if(c1.point.y() > c2.point.y())
		{
			qPointValue tmp = c1;
			c1 = c2;
			c2 = tmp;
		}
	}
	
	double pval[4] = { c1.value, c1.value, c2.value, c2.value };
	 
	double lineLength = boundLine.length();
	
	foreach(qPointValue v, inputs)
	{
		QLineF line;
		if(dir == 1)
			// line from point to TOP
			// (boundLine runs right-left on TOP side of bounds)
			line = QLineF(v.point, QPointF(v.point.x(), bounds.top()));
		else
		if(dir == 2)
			// line from point to RIGHT
			// (boundLine runs top-bottom on RIGHT side of bounds)
			line = QLineF(v.point, QPointF(bounds.right(), v.point.y()));
		else
		if(dir == 3)
			// line from point to BOTTOM
			// (boundLine runs right-left on BOTTOM side of bounds)
			line = QLineF(v.point, QPointF(v.point.x(), bounds.bottom()));
		else
		if(dir == 4)
			// line from point to LEFT
			// (boundLine runs top-bottom on LEFT side of bounds)
			line = QLineF(v.point, QPointF(bounds.left(),  v.point.y()));
		
		// test to see boundLine intersects line, if so, create point if none exists
		QPointF point;
		/*QLineF::IntersectType type = */boundLine.intersect(line, &point);
		if(!point.isNull())
		{
			if(!hasPoint(existingPoints, point) &&
			   !hasPoint(outputs, point))
			{
				double unitVal = (dir == 1 || dir == 3 ? v.point.x() : v.point.y()) / lineLength;
				double value = cubicInterpolate(pval, unitVal);
				outputs << qPointValue(point, value);
				//qDebug() << "testLine: "<<boundLine<<" -> "<<line<<" ["<<dir<<"/1] New point: "<<point<<", value:"<<value<<", corners: "<<c1.value<<","<<c2.value;
			}
		}
		
	}
	
	
	return outputs;
}

qPointValue nearestPoint(QList<qPointValue> list, QPointF point, int dir, bool requireValid=false)
{
	/// \a dir: 0=X+, 1=Y+, 2=X-, 3=Y-
	
	qPointValue minPnt;
	double minDist = (double)INT_MAX;
	double origin  = dir == 0 || dir == 2 ? point.x() : point.y();

	foreach(qPointValue v, list)
	{
		if(requireValid && !v.isValid())
			continue;
			
		double val  = dir == 0 || dir == 2 ? v.point.x() : v.point.y();
		double dist = fabs(val - origin);
		if(((dir == 0 && fuzzyIsEqual(v.point.y(), point.y()) && val > origin) ||
		    (dir == 1 && fuzzyIsEqual(v.point.x(), point.x()) && val > origin) ||
		    (dir == 2 && fuzzyIsEqual(v.point.y(), point.y()) && val < origin) ||
		    (dir == 3 && fuzzyIsEqual(v.point.x(), point.x()) && val < origin) )  &&
		    dist < minDist)
		{
			minPnt = v;
			minDist = dist;
		}
	}
	
	return minPnt;
}

QList<qPointValue> testInnerLines(QList<qPointValue> inputs, QList<qPointValue> existingPoints, QRectF bounds, QPointF p1, QPointF p2, int dir)
{
	QLineF boundLine(p1,p2);
	QList<qPointValue> outputs;
	
// 	qPointValue c1, c2;
// 	if(!hasPoint(existingPoints, boundLine.p1()) &&
// 	   !hasPoint(outputs,        boundLine.p1()))
// 	{
// 		c1 = qPointValue(boundLine.p1(), 0.); // NOTE assumingunknown corners have value of 0.
// 		outputs << c1;
// 	}
// 	else
// 	{
// 		c1 = getPoint(inputs, boundLine.p1());
// 	}
// 	
// 	if(!hasPoint(existingPoints, boundLine.p2()) &&
// 	   !hasPoint(outputs,        boundLine.p2()))
// 	{
// 		c2 = qPointValue(boundLine.p2(), 0.); // NOTE assuming unknown corners have value of 0.;
// 		outputs << c2;
// 	}
// 	else
// 	{
// 		c2 = getPoint(inputs, boundLine.p2());
// 	}
// 		
// 	double pval[4] = { c1.value, c1.value/2, c2.value/2, c2.value };
// 	 
// 	double lineLength = boundLine.length();
	
	foreach(qPointValue v, inputs)
	{
		QLineF line;
		if(dir == 1)
			// line from point to TOP
			// (boundLine runs right-left on TOP side of bounds)
			line = QLineF(v.point, QPointF(v.point.x(), bounds.top()));
		else
		if(dir == 2)
			// line from point to RIGHT
			// (boundLine runs top-bottom on RIGHT side of bounds)
			line = QLineF(v.point, QPointF(bounds.right(), v.point.y()));
		else
		if(dir == 3)
			// line from point to BOTTOM
			// (boundLine runs right-left on BOTTOM side of bounds)
			line = QLineF(v.point, QPointF(v.point.x(), bounds.bottom()));
		else
		if(dir == 4)
			// line from point to LEFT
			// (boundLine runs top-bottom on LEFT side of bounds)
			line = QLineF(v.point, QPointF(bounds.left(),  v.point.y()));
		
// 		// test to see boundLine intersects line, if so, create point if none exists
// 		QPointF point;
// 		/*QLineF::IntersectType type = */boundLine.intersect(line, &point);
// 		if(!point.isNull())
// 		{
// 			if(!hasPoint(existingPoints, point) &&
// 			   !hasPoint(outputs, point))
// 			{
// 				double unitVal = (dir == 1 || dir == 3 ? v.point.x() : v.point.y()) / lineLength;
// 				double value = cubicInterpolate(pval, unitVal);
// 				outputs << qPointValue(point, value);
// 				qDebug() << "testLine: "<<boundLine<<" -> "<<line<<" ["<<dir<<"/1] New point: "<<point<<", value:"<<value<<", corners: "<<c1.value<<","<<c2.value;
// 			}
// 		}
		

		foreach(qPointValue v2, inputs)
		{
			QLineF line2;
			if(dir == 1 || dir == 3)
				// 1=(boundLine runs right-left on TOP side of bounds)
				// 3=(boundLine runs right-left on BOTTOM side of bounds)
				// run line left right, see if intersects with 'line' above
				line2 = QLineF(bounds.left(), v2.point.y(), bounds.right(), v2.point.y());
			else
			if(dir == 2 || dir == 4)
				// 2=(boundLine runs top-bottom on RIGHT side of bounds)
				// 4=(boundLine runs top-bottom on LEFT side of bounds)
				// run line left right, see if intersects with 'line' above
				line2 = QLineF(v2.point.x(), bounds.top(), v2.point.x(), bounds.bottom());
			
			//qDebug() << "testLine: "<<boundLine<<" -> "<<line<<" ["<<dir<<"/2] Testing: "<<line2;
			
			// test to see boundLine intersects line, if so, create point if none exists
			QPointF point2;
			/*QLineF::IntersectType type = */line.intersect(line2, &point2);
			if(!point2.isNull())
			{
				if(!hasPoint(existingPoints, point2) &&
				   !hasPoint(outputs, point2))
				{
					// Intersected point, find nerest point +/-
					
// 					// Get values for the ends of the line (assuming interpolated above)
// 					qPointValue ic1 = getPoint(existingPoints, line2.p1());
// 					if(ic1.isNull())
// 						ic1 = getPoint(outputs, line2.p1());
// 					
// 					qPointValue ic2 = getPoint(existingPoints, line2.p2());
// 					if(ic2.isNull())
// 						ic2 = getPoint(outputs, line2.p2());
// 						
// 					if(ic1.isNull())
// 					{
// 						qDebug() << "ic1 logic error, quiting";
// 						exit(-1);
// 					}
// 					
// 					if(ic2.isNull())
// 					{
// 						qDebug() << "ic2 logic error, quiting";
// 						exit(-1);
// 					}

					QList<qPointValue> combined;
					combined << existingPoints << outputs;
					
					/// Third arg to nearestPoint() is a direction to search, 0-based: 0=X+, 1=Y+, 2=X-, 3=Y-
					
					qPointValue pntAfter  = nearestPoint(combined, point2, dir+1); // same dir
					qPointValue pntBefore = nearestPoint(combined, point2, dir-1); // oposite dir (dir is 1 based, nearestPoint expects 0 based)

					qPointValue pntPre = nearestPoint(combined, point2, //dir); // perpendicular positive (dir is 1 based, nearestPoint expects 0 based)
										dir == 1 ? 3 : // dir=1 is TL, arg=2 is X-
										dir == 2 ? 4 :
										dir == 3 ? 1 :
										dir == 4 ? 2 : 2);

					qPointValue pntPost = nearestPoint(combined, point2,
										dir == 1 ? 1 : // dir=1 is TL, arg=2 is X-
										dir == 2 ? 2 :
										dir == 3 ? 3 :
										dir == 4 ? 4 : 4);
										
					//double innerLineLength = QLineF(pntAfter.point, pntBefore.point).length();
					
					//double innerPval[4] = { pntBefore.value, pntBefore.value/2+pntBefore.value, pntAfter.value-pntAfter.value/2, pntAfter.value };
					//double innerPval[4] = { pntPre.value, pntBefore.value, pntAfter.value, pntAfter.value };
			
					// Note we are using perpendicular point components to the cubicInterpolate() call above
					//double unitVal = (dir == 1 || dir == 3 ? v.point.y() : v.point.x()) / innerLineLength;
					//double value = cubicInterpolate(innerPval, unitVal);

					QLineF line1 = QLineF(pntAfter.point, pntBefore.point);
					QLineF line2 = QLineF(pntPre.point,   pntPost.point);
// 					double lenLine1 = line1.length();
// 					double lenLine2 = line2.length();

					//double unit1 = (dir == 1 || dir == 3 ? v.point.y() : v.point.x()) / innerLineLength;

					double value;
					if(dir == 1 || dir == 3)
					{
						double unit11 = line2.y1() > line2.y2() ? line2.y2() : line2.y1(); // near
						double unit12 = line2.y1() > line2.y2() ? line2.y1() : line2.y2(); // far

						double val11  = line2.y1() > line2.y2() ? pntBefore.value : pntAfter.value;
						double val12  = line2.y1() > line2.y2() ? pntAfter.value : pntBefore.value;
						
						double unit21 = line1.x1() > line1.x2() ? line1.x2() : line1.x1(); // near
						double unit22 = line1.x1() > line1.x2() ? line1.x1() : line1.x2(); // far

						double val21  = line1.x1() > line1.x2() ? pntPre.value  : pntPost.value;
						double val22  = line1.x1() > line1.x2() ? pntPost.value : pntPre.value;


						double unitLen1 = unit12 - unit11;
						double unitLen2 = unit22 - unit21;

// 						double unit1 = (unit12 - point2.y()) / unitLen1;
// 						double unit2 = (unit22 - point2.x()) / unitLen1;
						
						double fr1 = (unit22 - point2.x()) / unitLen2 * val21
						           + (point2.x() - unit21) / unitLen2 * val22;
						
						//double fr2 = (corners[2].point.x() - x) / w * corners[3].value
						 //          + (x - corners[0].point.x()) / w * corners[2].value;
						
						double p1  = (unit12 - point2.y()) / unitLen1 * val11
						           + (point2.y() - unit11) / unitLen1 * val12;

						//double val1 = fr1 * (unit1 / 2.);
						//double val2 =  p1 * (unit2 / 2.);

						//value = val1 + val2;
						value = (fr1 + p1) / 2.;
						if(value < 0 || value > 1)
						{
							qDebug() << "** Value out of range: "<<value<<", calcs:";
							qDebug() << "\t dir[1||3]: point2:"<<point2<<", p1a:"<<((unit12 - point2.y()) / unitLen1) <<", p1b:" << ((point2.y() - unit11) / unitLen2);
							qDebug() << "\t dir[1||3]: unit1[1,2]:["<<unit11<<","<<unit12<<"], val1[1,2]:["<<val11<<","<<val12<<"]";
							qDebug() << "\t dir[1||3]: unit2[1,2]:["<<unit21<<","<<unit22<<"], val2[1,2]:["<<val21<<","<<val22<<"]";
							qDebug() << "\t dir[1||3]: unitLen[1,2]:["<<unitLen1<<","<<unitLen2/*<<"],unit[1,2]:["<<unit1<<","<<unit2*/<<"]";
							qDebug() << "\t dir[1||3]: fr1:"<<fr1<<", p1:"<<p1<</*", val1:"<<val1<<", val2:"<<val2<<*/", value:"<<value;
						}
					}
					else
					//if(dir == 1 || dir == 3)
					{
						double unit11 = line2.x1() > line2.x2() ? line2.x2() : line2.x1(); // near
						double unit12 = line2.x1() > line2.x2() ? line2.x1() : line2.x2(); // far
						
						double val11  = line2.x1() > line2.x2() ? pntBefore.value : pntAfter.value;
						double val12  = line2.x1() > line2.x2() ? pntAfter.value : pntBefore.value;
						
						double unit21 = line1.y1() > line1.y2() ? line1.y2() : line1.y1(); // near
						double unit22 = line1.y1() > line1.y2() ? line1.y1() : line1.y2(); // far
						
						double val21  = line1.y1() > line1.y2() ? pntPre.value  : pntPost.value;
						double val22  = line1.y1() > line1.y2() ? pntPost.value : pntPre.value;
						
						
						double unitLen1 = unit12 - unit11;
						double unitLen2 = unit22 - unit21;
						
// 						double unit1 = (unit12 - point2.y()) / unitLen1;
// 						double unit2 = (unit22 - point2.x()) / unitLen1;
						
						double fr1 = (unit22 - point2.y()) / unitLen2 * val21
						           + (point2.y() - unit21) / unitLen2 * val22;
						
						//double fr2 = (corners[2].point.x() - x) / w * corners[3].value
						//          + (x - corners[0].point.x()) / w * corners[2].value;
						
						double p1  = (unit12 - point2.x()) / unitLen1 * val11
						           + (point2.x() - unit11) / unitLen1 * val12;
						
						//double val1 = fr1 * (unit1 / 2.);
						//double val2 =  p1 * (unit2 / 2.);

						//value = val1 + val2;
						value = (fr1 + p1) / 2.;
					}

					if(value < 0 || value > 1)
					{
						qDebug() << "testLine: bound:"<<boundLine<<" -> line:"<<line<<" ["<<dir<<"/2]";
						qDebug() << "[inner] " << dir << ": New point: "<<point2<<" \t value:"<<QString().sprintf("%.02f",value).toDouble()<< "\n"
								<< " \t before/after: "<<pntBefore.value<<" @ " << pntBefore.point << " -> " << pntAfter.value << " @ " << pntAfter.point << "\n"
								<< " \t pre/post:     "<<pntPre.value   <<" @ " << pntPre.point    << " -> " << pntPost.value << " @ " << pntPost.point;
					}
					
					outputs << qPointValue(point2, isnan(value) ? 0 :value);
				}
			}
				
		}		
		
	}
	
	
	return outputs;
}


void SigMapRenderer::render()
{
	//qDebug() << "SigMapRenderer::render(): currentThreadId:"<<QThread::currentThreadId()<<", rendering...";
	QSize origSize = m_gs->m_bgPixmap.size();

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
	double dx=1., dy=1.;
	/*
	QImage mapImg(origSize, QImage::Format_ARGB32_Premultiplied);
	QPainter p(&mapImg);*/
	
 	QPicture pic;
 	QPainter p(&pic);
	p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
	//p.fillRect(mapImg.rect(), Qt::transparent);
	
	QHash<QString,QString> apMacToEssid;
	
	foreach(SigMapValue *val, m_gs->m_sigValues)
		foreach(WifiDataResult result, val->scanResults)
			if(!apMacToEssid.contains(result.mac))
				apMacToEssid.insert(result.mac, result.essid);
	
	//qDebug() << "SigMapRenderer::render(): Unique MACs: "<<apMacToEssid.keys()<<", mac->essid: "<<apMacToEssid;

	int numAps = apMacToEssid.keys().size();
	int idx = 0;
	
	(void)numAps; // avoid unused warnings
	
	if(m_gs->m_renderMode == MapGraphicsScene::RenderRectangles)
	{
		QRectF rect(0,0,300,300);
		//double w = rect.width(), h = rect.height();
	
		// Rectangle Bicubic Interpolation
		
		// The goal:
		// Given an arbitrary list of points (and values assoicated with each point),
		// interpolate the value for every point contained within the bounding rectangle.
		// Goal something like: http://en.wikipedia.org/wiki/File:BicubicInterpolationExample.png
		
		/*
		
		To illustrate what this means and how we do it, assume you give it four points (marked by asterisks, below),
		along with an arbitrary value for each point:
		
		*-----------------------
		|                       |
		|                       |
		|       *               |
		|                       |
		|                       |
		|                       |
		|                       |
		|             *         |
		|                       |
		|                       |
		-----------------------*
		
		The goal of the algorithm is to subdivide the bounding rectangle (described as the union of the location of all the points)
		into a series of sub-rectangles, and interpolate the values of the known points to the new points found by the intersections
		of the lines:
		
		X-------*-----*---------*
		|       |     |         |
		|       |     |         |
		*-------X-----*---------*
		|       |     |         |
		|       |     |         |
		|       |     |         |
		|       |     |         |
		*-------*-----X---------*
		|       |     |         |
		|       |     |         |
		*-------*-----*---------X
		
		Above, X marks the original points, and the asterisks marks the points the algorithm inserted to create the sub-rectangles.
		For each asterisk above, the algorithm also interpolated the values from each of the given points (X's) to find the value
		at the new point (using bicubic interpolation.)
		
		Now that the algorithm has a list of rectangles, it can procede to render each of them using bicubic interpolation of the value
		across the surface.
		
		Note that for any CORNER points which are NOT given, they are assumed to have a value of "0". This can be changed in "testLine()", above.
		*/
		
		
		// Setup our list of points - these can be in any order, any arrangement.
		
	// 	QList<qPointValue> points = QList<qPointValue>()
	// 		<< qPointValue(QPointF(0,0), 		0.0)
	// // 		<< qPointValue(QPointF(10,80), 		0.5)
	// // 		<< qPointValue(QPointF(80,10), 		0.75)
	// // 		<< qPointValue(QPointF(122,254), 	0.33)
	// 		<< qPointValue(QPointF(w,0), 		0.0)
	// 		<< qPointValue(QPointF(w/3*1,h/3*2),	1.0)
	// 		<< qPointValue(QPointF(w/2,h/2),	1.0)
	// 		<< qPointValue(QPointF(w/3*2,h/3*1),	1.0)
	// 		<< qPointValue(QPointF(w,h), 		0.0)
	// 		<< qPointValue(QPointF(0,h), 		0.0);
	
// 		QList<qPointValue> points = QList<qPointValue>()
// 			<< qPointValue(QPointF(0,0), 		0.5)
// 			//<< qPointValue(QPointF(w/3*2,h/3),	1.0)
// 			<< qPointValue(QPointF(w/2,h/2),	1.0)
// 			<< qPointValue(QPointF(w,w),		0.5);

// 		foreach(QString apMac, apMacToEssid.keys())
// 		{
		QString apMac = apMacToEssid.keys().at(0);
		qDebug() << "MAC: "<<apMac;
		QList<qPointValue> points;
		//MapApInfo *info = m_gs->apInfo(apMac);
			
		//QPointF center = info->point;
		
		foreach(SigMapValue *val, m_gs->m_sigValues)
			//val->consumed = false;
			if(val->hasAp(apMac))
			{
				qPointValue p = qPointValue( val->point, val->signalForAp(apMac) );
				points << p;
				qDebug() << "points << qPointValue(" << p.point << ", " << p.value << ");";
			}
		
	
	//  	#define val(x) ( ((double)x) / 6. )
	//  	QList<qPointValue> points = QList<qPointValue>()
	//  		<< qPointValue(QPointF(0,0),	val(0))
	//  		
	// 		<< qPointValue(QPointF(w/4*1,h/4*1),	val(1))
	// 		<< qPointValue(QPointF(w/4*2,h/4*1),	val(2))
	// 		<< qPointValue(QPointF(w/4*3,h/4*1),	val(4))
	// 		<< qPointValue(QPointF(w/4*4,h/4*1),	val(1))
	// 		
	// 		<< qPointValue(QPointF(w/4*1,h/4*2),	val(6))
	// 		<< qPointValue(QPointF(w/4*2,h/4*2),	val(3))
	// 		<< qPointValue(QPointF(w/4*3,h/4*2),	val(5))
	// 		<< qPointValue(QPointF(w/4*4,h/4*2),	val(2))
	// 		
	// 		<< qPointValue(QPointF(w/4*1,h/4*3),	val(4))
	// 		<< qPointValue(QPointF(w/4*2,h/4*3),	val(2))
	// 		<< qPointValue(QPointF(w/4*3,h/4*3),	val(1))
	// 		<< qPointValue(QPointF(w/4*4,h/4*3),	val(5))
	// 		
	// 		<< qPointValue(QPointF(w/4*1,h/4*4),	val(5))
	// 		<< qPointValue(QPointF(w/4*2,h/4*4),	val(4))
	// 		<< qPointValue(QPointF(w/4*3,h/4*4),	val(2))
	// 		<< qPointValue(QPointF(w/4*4,h/4*4),	val(3))
	// 		
	// 		;
	// 
	// 	#undef val
		
		
			
		
	
		// Sorted Y1->Y2 then X1->X2 (so for every Y, points go X1->X2, then next Y, etc, for example: (0,0), (1,0), (0,1), (1,1))
		qSort(points.begin(), points.end(), qPointValue_sort_point);
		
		// Find the bounds of the point cloud given so we can then subdivide it into smaller rectangles as needed:
		QRectF bounds;
		foreach(qPointValue v, points)
		{
			//qDebug() << "corner: "<<v.point<<", value:"<<v.value;
			bounds |= QRectF(v.point, QSizeF(1.,1.0));
		}
		bounds.adjust(0,0,-1,-1);
		
		// Iterate over every point
		// draw line from point to each bound (X1,Y),(X2,Y),(X,Y1),(X,Y2)
		// see if line intersects with perpendicular line from any other point
		// add point at intersection if none exists
		// add point at end point of line if none exists
		QList<qPointValue> outputList = points;
		
		outputList << testLine(points, outputList, bounds, bounds.topLeft(),		bounds.topRight(),     1);
		outputList << testLine(points, outputList, bounds, bounds.topRight(),		bounds.bottomRight(),  2);
		outputList << testLine(points, outputList, bounds, bounds.bottomRight(),	bounds.bottomLeft(),   3);
		outputList << testLine(points, outputList, bounds, bounds.bottomLeft(),		bounds.topLeft(),      4);
		
		/* After these four testLine() calls, the outputList now has these points (assuming the first example above):
		
		*-------X-----X---------X
		|       |     |         |
		|       |     |         |
		X-------*-----?---------X
		|       |     |         |
		|       |     |         |
		|       |     |         |
		|       |     |         |
		X-------?-----*---------X
		|       |     |         |
		|       |     |         |
		X-------X-----*---------*
		
		New points added by the four testLine() calls ar marked with an X.
		
		Note the two question marks above where the inner lines intersect - those points were NOT added because
		their values could not be interpolated until the values for the outer points were calculated.
		
		Now that every outer point along the outside edges of the outer rectangle have been calculated, we
		now can go thru and calculate the intersection of every line inside the rectangle.
		
		*/
			
		// 
		outputList << testInnerLines(points, outputList, bounds, bounds.topLeft(),	bounds.topRight(),     1);
		outputList << testInnerLines(points, outputList, bounds, bounds.topRight(),	bounds.bottomRight(),  2);
		outputList << testInnerLines(points, outputList, bounds, bounds.bottomRight(),	bounds.bottomLeft(),   3);
		outputList << testInnerLines(points, outputList, bounds, bounds.bottomLeft(),	bounds.topLeft(),      4);
		
		
		// Sort the list of ppoints so we procede thru the list top->bottom and left->right
		qSort(outputList.begin(), outputList.end(), qPointValue_sort_point);
		
		// Points are now sorted Y1->Y2 then X1->X2 (so for every Y, points go X1->X2, then next Y, etc, for example: (0,0), (1,0), (0,1), (1,1))
		
		
		qDebug() << "bounds:"<<bounds;
	
		QList<QList<qPointValue> > quads;
		
		QImage tmpImg(origSize, QImage::Format_ARGB32_Premultiplied);
		QPainter p2(&tmpImg);
		p2.fillRect(tmpImg.rect(), Qt::transparent);
		//p2.end();
		
// 		// Arbitrary rendering choices
// 	//	QImage img(w,h, QImage::Format_ARGB32_Premultiplied);
// 		QImage img(400,400, QImage::Format_ARGB32_Premultiplied);
// 		
// 		QPainter p(&img);
// 		// Move painter 50,50 so we have room for text (when debugging I rendered values over every point)
// 		p.fillRect(img.rect(), Qt::white);
// 	//	int topX=0, topY=0;
// 		int topX=50, topY=50;
// 		p.translate(topX,topY);
// 	
// 	// 	
		// This is the last stage of the algorithm - go thru the new point cloud and construct the actual sub-rectangles
		// by starting with each point and proceding clockwise around the rectangle, getting the nearest point in each direction (X+,Y), (X,Y+) and (X-,Y)
		foreach(qPointValue tl, outputList)
		{
			QList<qPointValue> quad;
			
			qPointValue tr = nearestPoint(outputList, tl.point, 0);
			qPointValue br = nearestPoint(outputList, tr.point, 1);
			qPointValue bl = nearestPoint(outputList, br.point, 2);
			
			// These 'isNull()' tests catch points on the edges of the bounding rectangle
			if(!tr.point.isNull() &&
			!br.point.isNull() &&
			!bl.point.isNull())
			{
				quad  << tl << tr << br << bl;
				quads << (quad);
				
	// 			qDebug() << "Quad[p]: "<<tl.point<<", "<<tr.point<<", "<<br.point<<", "<<bl.point;
	// 			qDebug() << "Quad[v]: "<<tl.value<<tr.value<<br.value<<bl.value;
			
				// Here's the actual rendering of the interpolated quad
				for(int y=(int)tl.point.y(); y<br.point.y(); y++)
				{
					QRgb* scanline = (QRgb*)tmpImg.scanLine(y);
					for(int x=(int)tl.point.x(); x<br.point.x(); x++)
					{
						double value = quadInterpolate(quad, (double)x, (double)y);
						QColor color = colorForValue(value);
						scanline[x] = color.rgba();
					}
				}
				
// 				QVector<QPointF> vec;
// 				vec <<tl.point<<tr.point<<br.point<<bl.point;
// 				//p2.setPen(QColor(0,0,0,127));
// 				p2.setPen(Qt::white);
// 				p2.drawPolygon(vec);
// 				
// 				p2.setPen(Qt::gray);
// 				p2.drawText(tl.point, QString().sprintf("%.02f",tl.value));
// 				p2.drawText(tr.point, QString().sprintf("%.02f",tr.value));
// 				p2.drawText(bl.point, QString().sprintf("%.02f",bl.value));
// 				p2.drawText(br.point, QString().sprintf("%.02f",br.value));
			}
		}
		
		//p.end();
		
		//exit(-1);
		/*
		
		// Just some debugging:
		
		QImage img(4,4, QImage::Format_ARGB32_Premultiplied);
		
		QList<QList<qPointValue> > quads;
		
		QList<qPointValue> points;
		
	// 	points = QList<qPointValue>()
	// 		<< qPointValue(QPointF(0,0), 					0.0)
	// 		<< qPointValue(QPointF(img.width()/2,0),			0.5)
	// 		<< qPointValue(QPointF(img.width()/2,img.height()), 		0.0)
	// 		<< qPointValue(QPointF(0,img.height()), 			0.5);
	// 	quads << points;
	// 	
	// 	points = QList<qPointValue>()
	// 		<< qPointValue(QPointF(img.width()/2,0), 		0.5)
	// 		<< qPointValue(QPointF(img.width(),0),			0.0)
	// 		<< qPointValue(QPointF(img.width(),img.height()), 	0.5)
	// 		<< qPointValue(QPointF(img.width()/2,img.height()), 	0.0);
	// 	quads << points;
	
	
		points = QList<qPointValue>()
			<< qPointValue(QPointF(0,0), 				0.0)
			<< qPointValue(QPointF(img.width()-1,0),		0.0)
			<< qPointValue(QPointF(img.width()-1,img.height()-1), 	1.0)
			<< qPointValue(QPointF(0,img.height()-1), 		0.0);
		quads << points;
		
		
		//quads << (points);
		foreach(QList<qPointValue> corners, quads)
		{
			//for(int y=0; y<img.height(); y++)
			for(int y=corners[0].point.y(); y<=corners[2].point.y(); y++)
			{
				QRgb* scanline = (QRgb*)img.scanLine(y);
				for(int x=corners[0].point.x(); x<=corners[2].point.x(); x++)
				{
					double value = quadInterpolate(corners, (double)x, (double)y);
					qDebug() << "pnt:"<<x<<","<<y<<", val:"<<value;
					QColor color = colorForValue(value);
					scanline[x] = color.rgba();
				}
			}
		}
		*/
		tmpImg.save("interpolate.jpg");
	
		//setPixmap(QPixmap::fromImage(img));//.scaled(200,200)));
		
		p.drawImage(0,0,tmpImg);
	}
	else
	if(m_gs->m_renderMode == MapGraphicsScene::RenderTriangles)
	{
		foreach(QString apMac, apMacToEssid.keys())
		{
			MapApInfo *info = m_gs->apInfo(apMac);
			
			QPointF center = info->point;
			
			foreach(SigMapValue *val, m_gs->m_sigValues)
				val->consumed = false;
			
			// Find first two closest points to center [0]
			// Make triangle
			// Find next cloest point
			// Make tri from last point, this point, center
			// Go on till all points done
			
			// Render first traingle
	// 		SigMapValue *lastPnt = findNearest(center, apMac);
	// 		if(lastPnt)
	// 		{
	// 			QImage tmpImg(origSize, QImage::Format_ARGB32_Premultiplied);
	// 			QPainter p2(&tmpImg);
	// 			p2.fillRect(tmpImg.rect(), Qt::transparent);
	// 			p2.end();
	// 			
	// 			SigMapValue *nextPnt = findNearest(lastPnt->point, apMac);
	// 			renderTriangle(&tmpImg, center, lastPnt, nextPnt, dx,dy, apMac);
	// 			
	// 			// Store this point to close the map 
	// 			SigMapValue *endPoint = nextPnt;
	// 			
	// 			// Render all triangles inside
	// 			while((nextPnt = findNearest(lastPnt->point, apMac)) != NULL)
	// 			{
	// 				renderTriangle(&tmpImg, center, lastPnt, nextPnt, dx,dy, apMac);
	// 				lastPnt = nextPnt;
	// 				//lastPnt = 0;//DEBUG
	// 			}
	// 			
	// 			// Close the map
	// 			renderTriangle(&tmpImg, center, lastPnt, endPoint, dx,dy, apMac);
	// 			
	// 			p.setOpacity(1. / (double)(numAps) * (numAps - idx));
	// 			p.drawImage(0,0,tmpImg);
	// 		
	// 		}
			
			
			QImage tmpImg(origSize, QImage::Format_ARGB32_Premultiplied);
			QPainter p2(&tmpImg);
			p2.fillRect(tmpImg.rect(), Qt::transparent);
			p2.end();
			
			SigMapValue *lastPnt;
			while((lastPnt = m_gs->findNearest(center, apMac)) != NULL)
			{
				SigMapValue *nextPnt = m_gs->findNearest(lastPnt->point, apMac);
				m_gs->renderTriangle(&tmpImg, center, lastPnt, nextPnt, dx,dy, apMac);
			}
			
			p.setOpacity(.5); //1. / (double)(numAps) * (numAps - idx));
			p.drawImage(0,0,tmpImg);
		
		
			idx ++;
		}
	
	}
	else
	{
		/*
		#ifdef Q_OS_ANDROID
		int steps = 4 * 4 * 2;
		#else
		int steps = 4 * 4 * 4; //4 * 2;
		#endif
		*/
		int steps = m_gs->m_renderOpts.radialCircleSteps;
		
		double angleStepSize = 360. / ((double)steps);
		
		//int radius = 64*10/2;
		
		/*
		#ifdef Q_OS_ANDROID
		const double levelInc = .50; //100 / 1;// / 2;// steps;
		#else
		const double levelInc = .25; //100 / 1;// / 2;// steps;
		#endif
		*/
		double levelInc = 100. / ((double)m_gs->m_renderOpts.radialLevelSteps);
		
		double totalProgress = (100./levelInc) * (double)steps * apMacToEssid.keys().size();
		double progressCounter = 0.;
		
		(void)totalProgress; // avoid "unused" warnings...
		
		if(m_gs->m_renderMode == MapGraphicsScene::RenderCircles)
		{
			totalProgress = apMacToEssid.keys().size() * m_gs->m_sigValues.size();
		}
		
		foreach(QString apMac, apMacToEssid.keys())
		{
			MapApInfo *info = m_gs->apInfo(apMac);
			
			if(!info->marked)
			{
				//qDebug() << "SigMapRenderer::render(): Unable to render signals for apMac:"<<apMac<<", AP not marked on MAP yet.";
				continue;
			}
			
			if(!info->renderOnMap)
			{
				qDebug() << "SigMapRenderer::render(): User disabled render for "<<apMac;
				continue;
			}
			
			foreach(SigMapValue *val, m_gs->m_sigValues)
				val->renderDataDirty = true;
			
			QPointF center = info->point;
			
			double maxDistFromCenter = -1;
			double maxSigValue = 0.0;
			foreach(SigMapValue *val, m_gs->m_sigValues)
			{
				if(val->hasAp(apMac))
				{
					double dx = val->point.x() - center.x();
					double dy = val->point.y() - center.y();
					double distFromCenter = sqrt(dx*dx + dy*dy);
					if(distFromCenter > maxDistFromCenter)
					{
						maxDistFromCenter = distFromCenter;
						maxSigValue = val->signalForAp(apMac);
					}
				}
			}
			
			
			//double circleRadius = maxDistFromCenter;
// 			double circleRadius = (1.0/maxSigValue) /** .75*/ * maxDistFromCenter;
			double circleRadius = maxDistFromCenter / maxSigValue;
			//maxDistFromCenter = circleRadius;
			//qDebug() << "SigMapRenderer::render(): circleRadius:"<<circleRadius<<", maxDistFromCenter:"<<maxDistFromCenter<<", maxSigValue:"<<maxSigValue;
			qDebug() << "render: "<<apMac<<": circleRadius: "<<circleRadius<<", center:"<<center;
			
			
			
			QRadialGradient rg;
			
			QColor centerColor = m_gs->colorForSignal(1.0, apMac);
			rg.setColorAt(0., centerColor);
			//qDebug() << "SigMapRenderer::render(): "<<apMac<<": sig:"<<1.0<<", color:"<<centerColor;
			
			if(m_gs->m_renderMode == MapGraphicsScene::RenderCircles)
			{
				if(!m_gs->m_renderOpts.multipleCircles)
				{
					foreach(SigMapValue *val, m_gs->m_sigValues)
					{
						if(val->hasAp(apMac))
						{
							double sig = val->signalForAp(apMac);
							QColor color = m_gs->colorForSignal(sig, apMac);
							rg.setColorAt(1-sig, color);
							
							//double dx = val->point.x() - center.x();
							//double dy = val->point.y() - center.y();
							//double distFromCenter = sqrt(dx*dx + dy*dy);
							
							//qDebug() << "SigMapRenderer::render(): "<<apMac<<": sig:"<<sig<<", color:"<<color<<", distFromCenter:"<<distFromCenter;
							
							
						}
	
						progressCounter ++;
						emit renderProgress(progressCounter / totalProgress);
						
						
					}
					
					rg.setCenter(center);
					rg.setFocalPoint(center);
					rg.setRadius(maxDistFromCenter);
					
					p.setOpacity(.75); //1. / (double)(numAps) * (numAps - idx));
					p.setOpacity(1. / (double)(numAps) * (numAps - idx));
					p.setBrush(QBrush(rg));
					//p.setBrush(Qt::green);
					p.setPen(QPen(Qt::black,1.5));
					//p.drawEllipse(0,0,iconSize,iconSize);
					p.setCompositionMode(QPainter::CompositionMode_Xor);
					p.drawEllipse(center, maxDistFromCenter, maxDistFromCenter);
					//qDebug() << "SigMapRenderer::render(): "<<apMac<<": center:"<<center<<", maxDistFromCenter:"<<maxDistFromCenter<<", circleRadius:"<<circleRadius;
				}
				else
				{
	
					SigMapRenderer_sort_apMac = apMac;
					SigMapRenderer_sort_center = center;
					
					QList<SigMapValue*> readings = m_gs->m_sigValues;
					
					// Sort the readings based on signal level so readings cloest to the antenna are drawn last
					qSort(readings.begin(), readings.end(), SigMapRenderer_sort_SigMapValue);
					
					foreach(SigMapValue *val, readings)
					{
						if(val->hasAp(apMac))
						{
							double sig = val->signalForAp(apMac);
							QColor color = m_gs->colorForSignal(sig, apMac);
	//						rg.setColorAt(1-sig, color);
							
							double dx = val->point.x() - center.x();
							double dy = val->point.y() - center.y();
	//  						double distFromCenter = sqrt(dx*dx + dy*dy);
							
							//qDebug() << "SigMapRenderer::render(): "<<apMac<<": sig:"<<sig<<", color:"<<color<<", distFromCenter:"<<distFromCenter;
							
							//p.drawEllipse(center, distFromCenter, distFromCenter);
	// 						rg.setCenter(center);
	// 						rg.setFocalPoint(center);
	// 						rg.setRadius(maxDistFromCenter);
							
							//p.setOpacity(.75); //1. / (double)(numAps) * (numAps - idx));
							p.setOpacity(.5); //1. / (double)(numAps) * (numAps - idx));
							if(m_gs->m_renderOpts.fillCircles)
								p.setBrush(color); //QBrush(rg));
							
							#ifdef Q_OS_ANDROID
							const double maxLineWidth = 7.5;
							#else
							const double maxLineWidth = 5.0;
							#endif
							
							if(m_gs->m_renderOpts.fillCircles)
								p.setPen(QPen(color.darker(400), (1.-sig)*maxLineWidth));
							else
								p.setPen(QPen(color, (1.-sig)*maxLineWidth));
			
			
							//p.setPen(QPen(Qt::black,1.5));
							
							p.drawEllipse(center, fabs(dx), fabs(dy));
						}
						
						progressCounter ++;
						emit renderProgress(progressCounter / totalProgress);
					}
				}
				
				//qDebug() << "SigMapRenderer::render(): "<<apMac<<": center:"<<center<<", maxDistFromCenter:"<<maxDistFromCenter;
			}
			else
			{
	
				#define levelStep2Point(levelNum,stepNum) QPointF( \
						(levelNum/100.*circleRadius) * cos(((double)stepNum) * angleStepSize * 0.0174532925) + center.x(), \
						(levelNum/100.*circleRadius) * sin(((double)stepNum) * angleStepSize * 0.0174532925) + center.y() )
				
				//qDebug() << "center: "<<center;
				
				
				QVector<QPointF> lastLevelPoints;
				for(double level=levelInc; level<=100.; level+=levelInc)
				{
					//qDebug() << "level:"<<level;
					QVector<QPointF> thisLevelPoints;
					QPointF lastPoint;
					for(int step=0; step<steps; step++)
					{
						QPointF realPoint = levelStep2Point(level,step);
						double renderLevel = m_gs->getRenderLevel(level,step * angleStepSize,realPoint,apMac,center,circleRadius);
						// + ((step/((double)steps)) * 5.); ////(10 * fabs(cos(step/((double)steps) * 359. * 0.0174532925)));// * cos(level/100. * 359 * 0.0174532925);
						
						QPointF here = levelStep2Point(renderLevel,step);
						
						if(step == 0) // && renderLevel > levelInc)
						{
							QPointF realPoint = levelStep2Point(level,(steps-1));
							
							double renderLevel = m_gs->getRenderLevel(level,(steps-1) * angleStepSize,realPoint,apMac,center,circleRadius);
							lastPoint = levelStep2Point(renderLevel,(steps-1));
							thisLevelPoints << lastPoint;
						}
		
						p.setOpacity(.8); //1. / (double)(numAps) * (numAps - idx));
						//p.setCompositionMode(QPainter::CompositionMode_Xor);
							
		//				p.setPen(QPen(centerColor.darker(), 4.0));
						#ifdef Q_OS_ANDROID
						const double maxLineWidth = 7.5;
						#else
						const double maxLineWidth = 15.0;
						#endif
						
						p.setPen(QPen(m_gs->colorForSignal((level)/100., apMac), (1.-(level/100.))*levelInc*maxLineWidth));
		
						p.drawLine(lastPoint,here);
						thisLevelPoints << here;
						
						lastPoint = here;
						
						progressCounter ++;
						emit renderProgress(progressCounter / totalProgress);
					}
					
					/*
					if(level > levelInc)
					{
						lastLevelPoints << thisLevelPoints;
						p.setBrush(m_gs->colorForSignal((level)/100., apMac));
						//p.drawPolygon(lastLevelPoints);
					}
					*/
					
					lastLevelPoints = thisLevelPoints;
				}
				
			
				
		// 		p.setCompositionMode(QPainter::CompositionMode_SourceOver);
		// 		
		// 		// Render lines to signal locations
		// 		p.setOpacity(.75);
		// 		p.setPen(QPen(centerColor, 1.5));
		// 		foreach(SigMapValue *val, m_gs->m_sigValues)
		// 		{
		// 			if(val->hasAp(apMac))
		// 			{
		// // 				double sig = val->signalForAp(apMac);
		// // 				QColor color = m_gs->colorForSignal(sig, apMac);
		// // 				rg.setColorAt(1-sig, color);
		// // 				
		// // 				qDebug() << "SigMapRenderer::render(): "<<apMac<<": sig:"<<sig<<", color:"<<color;
		// 				
		// 				p.setPen(QPen(centerColor, val->signalForAp(apMac) * 10.));
		// 				//p.drawLine(center, val->point);
		// 				
		// 				/// TODO Use radial code below to draw the line to the center of the circle for *this* AP
		// 				/*
		// 				#ifdef Q_OS_ANDROID
		// 					const int iconSize = 64;
		// 				#else
		// 					const int iconSize = 32;
		// 				#endif
		// 			
		// 				const double iconSizeHalf = ((double)iconSize)/2.;
		// 					
		// 				int numResults = results.size();
		// 				double angleStepSize = 360. / ((double)numResults);
		// 				
		// 				QRectF boundingRect;
		// 				QList<QRectF> iconRects;
		// 				for(int resultIdx=0; resultIdx<numResults; resultIdx++)
		// 				{
		// 					double rads = ((double)resultIdx) * angleStepSize * 0.0174532925;
		// 					double iconX = iconSizeHalf/2 * numResults * cos(rads);
		// 					double iconY = iconSizeHalf/2 * numResults * sin(rads);
		// 					QRectF iconRect = QRectF(iconX - iconSizeHalf, iconY - iconSizeHalf, (double)iconSize, (double)iconSize);
		// 					iconRects << iconRect;
		// 					boundingRect |= iconRect;
		// 				}
		// 				
		// 				boundingRect.adjust(-1,-1,+2,+2);
		// 				*/
		// 			}
		// 		}
			}
			
			idx ++;
		}
	}
	
	//mapImg.save("mapImg1.jpg");
	p.end();
	

	//m_gs->m_sigMapItem->setPixmap(QPixmap::fromImage(mapImg.scaled(origSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
	//emit renderComplete(mapImg);
// 	QPicture pic;
// 	QPainter pp(&pic);
// 	pp.drawImage(0,0,mapImg);
// 	pp.end();

	emit renderComplete(pic);
}

void MapGraphicsScene::renderProgress(double value)
{
	(void)value;
	//m_mapWindow->setStatusMessage(tr("Rendering %1%").arg((int)(value*100)), 250);
}

// void MapGraphicsScene::renderComplete(QImage mapImg)
// {
// 	qDebug() << "MapGraphicsScene::renderComplete(): currentThreadId:" << QThread::currentThreadId();
// 	m_sigMapItem->setPixmap(QPixmap::fromImage(mapImg));
// 	//m_sigMapItem->setPicture(mapImg);
// 	m_mapWindow->setStatusMessage(tr("Signal map updated"), 500);
// }

void MapGraphicsScene::renderComplete(QPicture pic)
{
	qDebug() << "MapGraphicsScene::renderComplete(): currentThreadId:" << QThread::currentThreadId();
	//m_sigMapItem->setPixmap(QPixmap::fromImage(mapImg));
	m_sigMapItem->setPicture(pic);
	m_mapWindow->setStatusMessage(tr("Signal map updated"), 500);
	
	if(m_firstRender) // first render after load, init view
	{
		m_mapWindow->gv()->reset();
		m_mapWindow->gv()->scaleView(m_viewScale);
		m_mapWindow->gv()->horizontalScrollBar()->setValue(m_scrollHPos);
		m_mapWindow->gv()->verticalScrollBar()->setValue(m_scrollVPos);
		m_firstRender = false;
	}
}

/// LongPressSpinner impl

LongPressSpinner::LongPressSpinner()
{
	m_goodPressFlag = false;
	m_progress = 0.;

	double iconSize = 64.;
#ifdef Q_OS_ANDROID
	iconSize= 192.;
#endif

	QSizeF size = QSizeF(iconSize,iconSize);//.expandTo(QApplication::globalStrut());

	//m_boundingRect = QRectF(QPointF(0,0),size);
	double fingerTipAdjust = 0;
 	#ifdef Q_OS_ANDROID
// 	// TODO Is there some OS preference that indicates right or left handed usage?
 	fingertipAdjust = (iconSize/2)/3;
	#endif

	m_boundingRect = QRectF(QPointF(-iconSize/2 - fingerTipAdjust,-iconSize/2),size);
	
	connect(&m_goodPressTimer, SIGNAL(timeout()), this, SLOT(fadeTick()));
	m_goodPressTimer.setInterval(333/20);
	//m_goodPressTimer.setSingleShot(true);
	
}

void LongPressSpinner::setGoodPress(bool flag)
{
	setVisible(true);
	setOpacity(1.0);
	prepareGeometryChange();
	m_goodPressFlag = flag;
	m_goodPressTimer.start();
	update();
}

void LongPressSpinner::fadeTick()
{
	setOpacity(opacity() - 20./333.);
	update();
	if(opacity() <= 0)
		goodPressExpire(); 
}

void LongPressSpinner::setVisible(bool flag)
{
	if(m_goodPressFlag)
		return;
	
	QGraphicsItem::setVisible(flag);
}

void LongPressSpinner::goodPressExpire()
{
	m_goodPressFlag = false;
	setOpacity(1.0);
	setVisible(false);
	prepareGeometryChange();
	update();
}

void LongPressSpinner::setProgress(double p)
{
	m_progress = p;
	setOpacity(1.0);
	setVisible(p > 0. && p < 1.);
	//qDebug() << "LongPressSpinner::setProgress(): m_progress:"<<m_progress<<", isVisible:"<<isVisible();
	update();
}

QRectF LongPressSpinner::boundingRect() const
{
	if(m_goodPressFlag)
	{
		int iconSize = 64/2;
#ifdef Q_OS_ANDROID
		iconSize = 128/2;
#endif
		return m_boundingRect.adjusted(-iconSize,-iconSize,iconSize,iconSize);
	}
	return m_boundingRect;
}
	
void LongPressSpinner::paint(QPainter *painter, const QStyleOptionGraphicsItem */*option*/, QWidget */*widget*/)
{
	painter->save();
	//painter->setClipRect( option->exposedRect );
	
	int iconSize = boundingRect().size().toSize().width();
	//painter->translate(-iconSize/2,-iconSize/2);
	
	if(m_goodPressFlag)
	{
		// Draw inner gradient
		QColor centerColor = Qt::green;
		QRadialGradient rg(boundingRect().center(),iconSize);
		rg.setColorAt(0, centerColor/*.lighter(100)*/);
		rg.setColorAt(1, centerColor.darker(500));
		//p.setPen(Qt::black);
		painter->setBrush(QBrush(rg));
		
		//painter->setPen(QPen(Qt::black,3));
		painter->drawEllipse(boundingRect());
	}
	else
	{
		painter->setOpacity(0.75);
		
		// Draw inner gradient
		QColor centerColor("#0277fd"); // cream blue
		QRadialGradient rg(boundingRect().center(),iconSize);
		rg.setColorAt(0, centerColor/*.lighter(100)*/);
		rg.setColorAt(1, centerColor.darker(500));
		//p.setPen(Qt::black);
		painter->setBrush(QBrush(rg));
		
		//qDebug() << "LongPressSpinner::paint(): progress:"<<m_progress<<", rect:"<<boundingRect()<<", iconSize:"<<iconSize;
		
		QPainterPath outerPath;
		outerPath.addEllipse(boundingRect());
		
		QPainterPath innerPath;
		innerPath.addEllipse(boundingRect().adjusted(12.5,12.5,-12.5,-12.5));
		
		// Clip center of circle
		painter->setClipPath(outerPath.subtracted(innerPath));
		
		// Draw outline
		painter->setPen(QPen(Qt::black,3));
		//painter->drawEllipse(0,0,iconSize,iconSize);
		painter->drawChord(boundingRect().adjusted(3,3,-3,-3), 
				0, -(int)(360 * 16 * m_progress)); // clockwise
	
		painter->setBrush(Qt::white);
		painter->drawChord(boundingRect().adjusted(10,10,-10,-10), 
				0, -(int)(360 * 16 * m_progress)); // clocwise
	}
	
	painter->restore();
}

/// End SigMapItem impl


/// SigMapItem impl

SigMapItem::SigMapItem()
{
	m_internalCache = true;
}

void SigMapItem::setInternalCache(bool flag)
{
	m_internalCache = flag;
	setPicture(m_pic);
}

void SigMapItem::setPicture(QPicture pic)
{
	setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	//setCacheMode(QGraphicsItem::ItemCoordinateCache);
	prepareGeometryChange();
	m_pic = pic;
	
	m_offset = QPoint();
	
	if(m_internalCache)
	{
		QRect picRect = m_pic.boundingRect();
		if(picRect.isValid())
		{
			m_offset = picRect.topLeft();
			
			QImage img(picRect.size(), QImage::Format_ARGB32_Premultiplied);
			QPainter p(&img);
			p.fillRect(img.rect(), Qt::transparent);
			p.drawPicture(-m_offset, m_pic);
			p.end();
			m_img = img;
		}
		
		//qDebug() << "SigMapItem::setPicture(): m_img.size():"<<m_img.size()<<", picRect:"<<picRect;
		//m_img.save("mapImg.jpg");
	}
	else
		m_img = QImage();
	
	update();
}
	
// void SigMapItem::setPicture(QImage img)
// {
// 	m_img = img;
// 	update();
// }


QRectF SigMapItem::boundingRect() const
{
	return m_pic.boundingRect();
	//return m_img.rect();	
}
	
void SigMapItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget */*widget*/)
{
	//qDebug() << "SigMapItem::paint: option->exposedRect:"<<option->exposedRect<<", m_offset:"<<m_offset<<", m_img.size():"<<m_img.size();
		
	painter->save();
	//painter->setOpacity(0.75);
	painter->setClipRect( option->exposedRect );
	
	if(m_internalCache && 
	  !m_img.isNull())
	{
		/*
		if(option->exposedRect.isValid())
			painter->drawImage( option->exposedRect.translated(m_offset), m_img, option->exposedRect.toRect() );
		else
		*/
			painter->drawImage( m_offset, m_img );
	}
	else
	if(!m_pic.isNull())
		painter->drawPicture( m_offset, m_pic );

// 	if(!m_img.isNull())
// 	{
// // 		if(option->exposedRect.isValid())
// // 			painter->drawImage( option->exposedRect, m_img, option->exposedRect.toRect() );
// // 		else
// 			painter->drawImage( 0, 0, m_img );
// 	}

	painter->restore();
}

/// End SigMapItem impl

void MapGraphicsScene::setRenderAp(MapApInfo *info, bool flag)
{
	info->renderOnMap = flag;
	triggerRender();
}

void MapGraphicsScene::setRenderAp(QString mac, bool flag)
{
	apInfo(mac)->renderOnMap = flag;
	triggerRender();
}

MapApInfo* MapGraphicsScene::apInfo(WifiDataResult r)
{
	if(m_apInfo.contains(r.mac))
		return m_apInfo.value(r.mac);
		
	MapApInfo *inf = new MapApInfo(r);
	m_apInfo[r.mac] = inf;
	 
	return inf;
}


MapApInfo* MapGraphicsScene::apInfo(QString apMac)
{
	if(m_apInfo.contains(apMac))
		return m_apInfo.value(apMac);
		
	MapApInfo *inf = new MapApInfo();
	inf->mac = apMac;
	m_apInfo[apMac] = inf;
	 
	return inf;
}

QColor MapGraphicsScene::baseColorForAp(QString apMac)
{
	QColor baseColor;
	
	MapApInfo *info = apInfo(apMac);
	
	if(info->color.isValid())
	{
		baseColor = info->color;
		//qDebug() << "MapGraphicsScene::colorForSignal: "<<apMac<<": Loaded base "<<baseColor<<" from m_apMasterColor";
	} 
	else
	{
		bool foundColor = false;
		while(m_colorCounter < m_masterColorsAvailable.size()-1)
		{
			baseColor = m_masterColorsAvailable[m_colorCounter ++];
			
			bool colorInUse = false;
			foreach(MapApInfo *info, m_apInfo)
				if(info->color == baseColor)
					colorInUse = true;
					
			if(!colorInUse)
			{
				foundColor = true;
				break;
			}
		}
		
		if(foundColor)
		{
			qDebug() << "MapGraphicsScene::baseColorForAp: "<<apMac<<": Using NEW base "<<baseColor.name()<<" from m_masterColorsAvailable # "<<m_colorCounter;
		}
		//if(!foundColor)
		else
		{
			baseColor = QColor::fromHsv(qrand() % 359, 255, 255);
			qDebug() << "MapGraphicsScene::baseColorForAp: "<<apMac<<": Using NEW base "<<baseColor.name()<<" (random HSV color)";
		}
		
		//m_apMasterColor[apMac] = baseColor;
		info->color = baseColor;
		//qDebug() << "MapGraphicsScene::colorForSignal: "<<apMac<<": Stored baseColor "<<m_apMasterColor[apMac]<<" in m_apMasterColor["<<apMac<<"]";
	}
	
	return baseColor;
}

QColor MapGraphicsScene::colorForSignal(double sig, QString apMac)
{
	QList<QColor> colorList;
	if(!m_colorListForMac.contains(apMac))
	{
		//qDebug() << "MapGraphicsScene::colorForSignal: "<<apMac<<": m_colorListforMac cache miss, getting base...";
		QColor baseColor = baseColorForAp(apMac);
		
		// paint the gradient
		#ifdef Q_OS_ANDROID
		int imgHeight = 1;
		#else
		int imgHeight = 10; // for debugging output
		#endif
		
		QImage signalLevelImage(100,imgHeight,QImage::Format_ARGB32_Premultiplied);
		QPainter sigPainter(&signalLevelImage);
		
		if(m_renderMode == RenderCircles ||
		   m_renderMode == RenderTriangles)
		{
			QLinearGradient fade(QPoint(0,0),QPoint(100,0));
	// 		fade.setColorAt( 0.3, Qt::black  );
	// 		fade.setColorAt( 1.0, Qt::white  );
			fade.setColorAt( 0.3, Qt::red    );
			fade.setColorAt( 0.7, Qt::yellow );
			fade.setColorAt( 1.0, Qt::green  );
			sigPainter.fillRect( signalLevelImage.rect(), fade );
			sigPainter.setCompositionMode(QPainter::CompositionMode_HardLight);
			sigPainter.fillRect( signalLevelImage.rect(), baseColor ); 
		
		}
		else
		{
			//sigPainter.fillRect( signalLevelImage.rect(), baseColor ); 
		
			QLinearGradient fade(QPoint(0,0),QPoint(100,0));
			fade.setColorAt( 1.0, baseColor.lighter(100)  );
	 		fade.setColorAt( 0.5, baseColor               );
	 		//fade.setColorAt( 0.0, Qt::transparent  );
	 		fade.setColorAt( 0.0, baseColor.darker(300)   );
			sigPainter.fillRect( signalLevelImage.rect(), fade );
			//sigPainter.setCompositionMode(QPainter::CompositionMode_HardLight);
			//sigPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		}
		
		sigPainter.end();
		
		#ifndef Q_OS_ANDROID
		// just for debugging (that's why the img is 10px high - so it is easier to see in this output)
		signalLevelImage.save(tr("signalLevelImage-%1.png").arg(apMac));
		#endif
		
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
	QFileInfo info(filename);
	if(info.exists() && !info.isWritable())
	{
		QMessageBox::critical(0, "Unable to Save", tr("Unable to save %1 - it's not writable!").arg(filename));
		return;
	}
	
	QSettings data(filename, QSettings::IniFormat);
	
	qDebug() << "MapGraphicsScene::saveResults(): Saving to: "<<filename;
	
	// Store bg filename
	data.setValue("background", m_bgFilename);
	
	// Store window scroll/zoom location
	data.setValue("h-pos", m_mapWindow->gv()->horizontalScrollBar()->value());
	data.setValue("v-pos", m_mapWindow->gv()->verticalScrollBar()->value());
	data.setValue("scale", m_mapWindow->gv()->scaleFactor());
	
	// Store view cale translation values
	/// TODO make user configurable
	data.setValue("footpx",  m_footPx);
	data.setValue("meterpx", m_meterPx);
	
	int idx = 0;
	
	// Save AP locations
	data.beginWriteArray("ap-locations");
	foreach(MapApInfo *info, m_apInfo)
	{
		data.setArrayIndex(idx++);
		data.setValue("mac",		info->mac);
		data.setValue("essid",		info->essid);
		data.setValue("marked",		info->marked);
		data.setValue("point",		qPointFToString(info->point));
		data.setValue("color", 		info->color.name());
		data.setValue("renderap",	info->renderOnMap);
		data.setValue("mfg",		info->mfg);
		data.setValue("txpower",	info->txPower);
		data.setValue("txgain",		info->txGain);
		data.setValue("lossfactor",	qPointFToString(info->lossFactor));
		data.setValue("shortcutoff",	info->shortCutoff);
	}
	data.endArray();
		
	// Save signal readings
	idx = 0;
	data.beginWriteArray("readings");
	foreach(SigMapValue *val, m_sigValues)
	{
		data.setArrayIndex(idx++);
		data.setValue("point",		qPointFToString(val->point));
		data.setValue("rxgain",		val->rxGain);
		data.setValue("rxmac",		val->rxMac);
		data.setValue("timestamp",	val->timestamp.toTime_t());
		data.setValue("lat",		val->lat);
		data.setValue("lng",		val->lng);
		
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
	//m_mapWindow->gv()->resetMatrix();
	m_mapWindow->gv()->resetTransform();
	
	m_colorCounter = 0; // reset AP color counter
	
	QSettings data(filename, QSettings::IniFormat);
	qDebug() << "MapGraphicsScene::loadResults(): Loading from: "<<filename;
	
	QFileInfo info(filename);
	if(info.exists() && !info.isWritable())
	{
		QMessageBox::warning(0, "Warning", tr("%1 is READ ONLY - you won't be able to save any changes!").arg(filename));
	}
	
	
	clear(); // clear and reset the map
	
	// Load background file
	QString bg = data.value("background").toString();
	if(!bg.isEmpty())
		setBgFile(bg);
		
	// Load scroll/zoom locations
	double scale = data.value("scale", 1.0).toDouble();
	m_mapWindow->gv()->scaleView(scale);
	
	int hPos = data.value("h-pos", 0).toInt();
	int vPos = data.value("v-pos", 0).toInt();
	m_mapWindow->gv()->horizontalScrollBar()->setValue(hPos);
	m_mapWindow->gv()->verticalScrollBar()->setValue(vPos);
	
	// Store for setting up the view after render because
	// the real "pixel meaning" of hPos may change if the render resizes the scene rect,
	// so reapply after the scene rect may have changed for the first time.
	m_firstRender = true;
	m_scrollHPos = hPos;
	m_scrollVPos = vPos;
	m_viewScale = scale;
	
	// Read scale translation values
	m_footPx  = data.value("footpx",  13.).toDouble();
	m_meterPx = data.value("meterpx", 42.).toDouble();
	
	// Load ap locations
	int numApLocations = data.beginReadArray("ap-locations");
	
	qDebug() << "MapGraphicsScene::loadResults(): Reading numApLocations: "<<numApLocations;
	for(int i=0; i<numApLocations; i++)
	{
		data.setArrayIndex(i);
		
		QString pointString = data.value("point").toString();
		if(pointString.isEmpty())
			// prior to r38, point key was "center"
			pointString = data.value("center").toString();
		
		QPointF point  = qPointFFromString(pointString);
		QString apMac  = data.value("mac").toString();
		QString essid  = data.value("essid").toString();
		bool marked    = data.value("marked", !point.isNull()).toBool();
		QColor color   = data.value("color").toString();
		bool render    = data.value("renderap", true).toBool();
		
		QString mfg    = data.value("mfg", "").toString(); // TODO build chart of [txpower,txgain] based on mfg [which can be drived from MAC OUI]
		double txPower = data.value("txpower",	11.8).toDouble();  // dBm, 11.9 dBm = 19 mW (approx) = Linksys WRT54 Default TX power (approx)
		double txGain  = data.value("txgain",    5.0).toDouble();  // dBi,  5.0 dBi = Linksys WRT54 Stock Antenna Gain (approx)
		QPointF lossf  = qPointFFromString(data.value("lossfactor", "").toString());
				// lossfactor.x() and .y() are arbitrary tuning parameters X and Y, typically in the range of [-1.0,5.0]
				// (formula variable 'n' for short and far dBms, respectively)
		int shortCutoff = data.value("shortcutoff", -49).toInt();
				   // dBm (typically -49) value at which to swith from loss factor X to loss factor Y for calculating distance 
				   // (less than shortCutoff [close to AP], use X, greater [farther from AP] - use Y)
		
		if(!color.isValid())
		{
			qDebug() << "MapGraphicsScene::loadResults(): Getting base color for AP#:"<<i<<", mac:"<<apMac;
			color = baseColorForAp(apMac);
		}
		
		MapApInfo *info = apInfo(apMac);
		
		info->essid	= essid;
		info->marked	= marked;
		info->point	= point;
		info->color	= color;
		info->renderOnMap = render;
		info->mfg	= mfg;
		info->txPower	= txPower;
		info->txGain	= txGain;
		info->lossFactor = lossf;
		info->shortCutoff = shortCutoff;
		
		if(marked)
			addApMarker(point, apMac);
	}
	data.endArray();
	
	// Load signal readings
	int numReadings = data.beginReadArray("readings");
	qDebug() << "MapGraphicsScene::loadResults(): Reading numReadings: "<<numReadings;
	for(int i=0; i<numReadings; i++)
	{
		/*
		qDebug() << "MapGraphicsScene::loadResults(): i: "<<i<<" / "<<numReadings;
		if(i != numReadings - 3)
			continue; /// NOTE just for debugging triangulation/trilateration - REMOVE for production!
		*/	
		data.setArrayIndex(i);
		//qDebug() << "MapGraphicsScene::loadResults(): Reading point#: "<<i;
		
		// Load location of this reading
		QPointF point = qPointFFromString(data.value("point").toString());
		
		// Load other fields
		double rxGain  = data.value("rxgain",-3.).toDouble();
		QString rxMac  = data.value("rxMac","").toString();
		uint timestamp = data.value("timestamp",0).toUInt();
		double lat     = data.value("lat",0.).toDouble();
		double lng     = data.value("lng",0.).toDouble();
		
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
			
			// Patch old data files
			MapApInfo *info = apInfo(result.mac);
			if(info->essid.isEmpty())
				info->essid = result.essid;
		}
		data.endArray();
		
		SigMapValue *val = addSignalMarker(point, results);
		
		val->rxGain	= rxGain;
		val->rxMac	= rxMac;
		val->timestamp	= QDateTime::fromTime_t(timestamp);
		val->lat	= lat;
		val->lng	= lng;
		
	}
	data.endArray();
	
	// Call for render of map overlay
	triggerRender();
}



void MapGraphicsScene::renderTriangle(QImage *img, SigMapValue *a, SigMapValue *b, SigMapValue *c, double dx, double dy, QString apMac)
{
	if(!a || !b || !c)
		return;

/*	QVector<QPointF> points = QVector<QPointF>()
		<< QPointF(10, 10)
		<< QPointF(imgWidth - 10, imgHeight / 2)
		<< QPointF(imgWidth / 2,  imgHeight - 10);
		
	QList<QColor> colors = QList<QColor>()
		<< Qt::red
		<< Qt::green
		<< Qt::blue;*/
	
	QVector<QPointF> points = QVector<QPointF>()
		<< QPointF(a->point.x() * dx, a->point.y() * dy)
		<< QPointF(b->point.x() * dx, b->point.y() * dy)
		<< QPointF(c->point.x() * dx, c->point.y() * dy);
		
	QList<QColor> colors = QList<QColor>()
		<< colorForSignal(a->signalForAp(apMac), apMac)
		<< colorForSignal(b->signalForAp(apMac), apMac)
		<< colorForSignal(c->signalForAp(apMac), apMac);

	qDebug() << "MapGraphicsScene::renderTriangle[1]: "<<apMac<<": "<<points;
	if(!fillTriColor(img, points, colors))
		qDebug() << "MapGraphicsScene::renderTriangle[1]: "<<apMac<<": Not rendered";
		
// 	QPainter p(img);
// 	//p.setBrush(colors[0]);
// 	p.setPen(QPen(colors[2],3.0));
// 	p.drawConvexPolygon(points);
// 	p.end();

}

void MapGraphicsScene::renderTriangle(QImage *img, QPointF center, SigMapValue *b, SigMapValue *c, double dx, double dy, QString apMac)
{
	if(!b || !c)
		return;

/*	QVector<QPointF> points = QVector<QPointF>()
		<< QPointF(10, 10)
		<< QPointF(imgWidth - 10, imgHeight / 2)
		<< QPointF(imgWidth / 2,  imgHeight - 10);
		
	QList<QColor> colors = QList<QColor>()
		<< Qt::red
		<< Qt::green
		<< Qt::blue;*/
	
	QVector<QPointF> points = QVector<QPointF>()
		<< QPointF(center.x() * dx, center.y() * dy)
		<< QPointF(b->point.x() * dx, b->point.y() * dy)
		<< QPointF(c->point.x() * dx, c->point.y() * dy);
		
	QList<QColor> colors = QList<QColor>()
		<< colorForSignal(1.0, apMac)
		<< colorForSignal(b->signalForAp(apMac), apMac)
		<< colorForSignal(c->signalForAp(apMac), apMac);

	qDebug() << "MapGraphicsScene::renderTriangle[2]: "<<apMac<<": "<<points;
	if(!fillTriColor(img, points, colors))
		qDebug() << "MapGraphicsScene::renderTriangle[2]: "<<apMac<<": Not rendered";
		
// 	QPainter p(img);
// 	//p.setBrush(colors[0]);
// 	p.setPen(QPen(colors[2],3.0));
// 	p.drawConvexPolygon(points);
// 	p.end();

}

void MapGraphicsScene::setRenderOpts(MapRenderOptions opts)
{
	m_renderOpts = opts;
	/*
	//Defaults:
	m_renderOpts.cacheMapRender     = true;
	m_renderOpts.showReadingMarkers = true;
	m_renderOpts.multipleCircles	= true;
	m_renderOpts.fillCircles	= true;
	m_renderOpts.radialCircleSteps	= 4 * 4 * 4;
	m_renderOpts.radialLevelSteps	= (int)(100 / .25);
	m_renderOpts.radialAngleDiff	= 45 * 3;
	m_renderOpts.radialLevelDiff	= 100 / 3;
	m_renderOpts.radialLineWeight	= 200;
	*/
	
	m_sigMapItem->setInternalCache(m_renderOpts.cacheMapRender);
	
	foreach(SigMapValue *val, m_sigValues)
		val->marker->setVisible(m_renderOpts.showReadingMarkers);
		
	triggerRender();	
}


void MapGraphicsScene::setMeterPx(double m)
{
	m_meterPx = m;
	m_footPx  = m / 3.28084;
}

void MapGraphicsScene::setShowMyLocation(bool flag)
{
	m_showMyLocation = flag;
	QSettings("wifisigmap").setValue("showMyLocation", flag);
}

void MapGraphicsScene::setAutoGuessApLocations(bool flag)
{
	m_autoGuessApLocations = flag;
	QSettings("wifisigmap").setValue("autoGuessApLocations", flag);
}

