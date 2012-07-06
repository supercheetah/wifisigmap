#include "MapWindow.h"

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

MapGraphicsView::MapGraphicsView()
	: QGraphicsView()
{
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
	dynamic_cast<MapGraphicsScene*>(scene())->invalidateLongPress();
		
	QGraphicsView::mouseMoveEvent(mouseEvent);
}


MapWindow::MapWindow(QWidget *parent)
	: QWidget(parent)
{
	setWindowTitle("WiFi Signal Mapper");
	setupUi();
}

void MapWindow::setupUi()
{
	QVBoxLayout *vbox = new QVBoxLayout(this);
	
	//QGraphicsView *gv = new QGraphicsView();
	MapGraphicsView *gv = new MapGraphicsView();
	//gv->setDragMode(QGraphicsView::ScrollHandDrag);
	vbox->addWidget(gv);
	
	m_scene = new MapGraphicsScene();
	gv->setScene(m_scene);
}


MapGraphicsScene::MapGraphicsScene()
{
	//setBackgroundBrush(QImage("PCI-PlantLayout-20120705-2048px-adjusted.png"));
	m_bgPixmap = QPixmap("PCI-PlantLayout-20120705-2048px-adjusted.png");
	QGraphicsItem *item = addPixmap(m_bgPixmap);
	item->setZValue(0);
	
	connect(&m_longPressTimer, SIGNAL(timeout()), this, SLOT(longPressTimeout()));
	m_longPressTimer.setInterval(1000);
	m_longPressTimer.setSingleShot(true);
	
	QPixmap empty(m_bgPixmap.size());
	empty.fill(Qt::transparent);
	m_sigMapItem = addPixmap(empty);
	m_sigMapItem->setZValue(1);
	m_sigMapItem->setOpacity(0.75);
	
	
	// paint the gradient
	QImage signalLevelImage(100,10,QImage::Format_ARGB32_Premultiplied);
	QPainter sigPainter(&signalLevelImage);
	QLinearGradient fade(QPoint(0,0),QPoint(100,0));
	fade.setColorAt( 0.3, Qt::red    );
	fade.setColorAt( 0.7, Qt::yellow );
	fade.setColorAt( 1.0, Qt::green  );
	sigPainter.fillRect( signalLevelImage.rect(), fade );
	sigPainter.end();
	
	// just for debugging (that's why the img is 10px high - so it is easier to see in this output)
	signalLevelImage.save("signalLevelImage.png");
	
	QRgb* scanline = (QRgb*)signalLevelImage.scanLine(0);
	for(int i=0; i<100; i++)
	{
		QColor color;
		color.setRgba(scanline[i]);
		m_colorList << color;
	}
	
// 	QSizeF sz = m_bgPixmap.size();
// 	double w = sz.width();
// 	double h = sz.height();
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
	bool ok;
	int i = QInputDialog::getInt(0, tr("Input Strength"),
					tr("Strength (%):"), 25, 0, 100, 1, &ok);
	if(ok)
	{
		addSignalMarker(m_pressPnt, ((double)i)/100.);
		QTimer::singleShot(0, this, SLOT(renderSigMap()));
	}
}

void MapGraphicsScene::addSignalMarker(QPointF point, double value)
{	
	//QPixmap icon("red-select2.png");
	QColor centerColor = colorForSignal(value);
	
	int iconSize = 16;
	
	QImage marker(iconSize+1,iconSize+1,QImage::Format_ARGB32_Premultiplied);
	marker.fill(Qt::transparent);
	QPainter p(&marker);
	
	//p.save();
	//p.translate(-iconSize,-iconSize);
	QRadialGradient rg(QPointF(iconSize/2,iconSize/2),iconSize);
	rg.setColorAt(0, centerColor);
	rg.setColorAt(1, centerColor.darker());
	p.setPen(Qt::black);
	p.setBrush(QBrush(rg));
	p.drawEllipse(0,0,iconSize,iconSize);
	//p.restore();
	/*
	p.setPen(Qt::green);
	p.setBrush(QBrush());
	p.drawRect(marker.rect());
	*/
	p.end();
	
	QGraphicsItem *item = addPixmap(QPixmap::fromImage(marker));
	
	double w2 = (double)(marker.width())/2.;
	double h2 = (double)(marker.height())/2.;
	QPointF pnt(point.x()-w2,point.y()-h2);
	item->setPos(QPointF(pnt));
	
	item->setFlag(QGraphicsItem::ItemIsMovable);
	item->setFlag(QGraphicsItem::ItemSendsGeometryChanges);
	item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	item->setZValue(99);
		
	m_sigValues << new SigMapValue(point, value);
}

SigMapValue *MapGraphicsScene::findNearest(SigMapValue *match)
{
	if(!match)
		return 0;
		
	SigMapValue *nearest = 0;
	double minDist = (double)INT_MAX;
	
	foreach(SigMapValue *val, m_sigValues)
	{
		if(val->consumed)
			continue;
			
		double dx = val->point.x() - match->point.x();
		double dy = val->point.y() - match->point.y(); 
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

void MapGraphicsScene::renderSigMap()
{
	QSize origSize = m_bgPixmap.size();
	QSize renderSize = origSize;
	renderSize.scale(origSize.width() * .33, origSize.height() * .33, Qt::KeepAspectRatio);
	
	double dx = ((double)renderSize.width())  / ((double)origSize.width());
	double dy = ((double)renderSize.height()) / ((double)origSize.height());
	
	QImage mapImg(renderSize, QImage::Format_ARGB32_Premultiplied);
	QPainter p(&mapImg);
	p.fillRect(mapImg.rect(), Qt::transparent);
	p.end();
	
	//QVector<QPointF> points, QList<QColor> colors
// 	double imgWidth  = mapImg.width();
// 	double imgHeight = mapImg.height();

	foreach(SigMapValue *val, m_sigValues)
		val->consumed = false;
	
	m_sigValues[0]->consumed = true; // center
	
	// Find first two closest points to center [0]
	// Make triangle
	// Find next cloest point
	// Make tri from last point, this point, center
	// Go on till all points done
	
	// Render first traingle
	SigMapValue *lastPnt = findNearest(m_sigValues[0]);
	SigMapValue *nextPnt = findNearest(lastPnt);
	renderTriangle(&mapImg, m_sigValues[0], lastPnt, nextPnt, dx,dy);
	
	// Store this point to close the map 
	SigMapValue *endPoint = nextPnt;
	
	// Render all triangles inside
	while((nextPnt = findNearest(lastPnt)) != NULL)
	{
		renderTriangle(&mapImg, m_sigValues[0], lastPnt, nextPnt, dx,dy);
		lastPnt = nextPnt;
		//lastPnt = 0;//DEBUG
	}
	
	// Close the map
	renderTriangle(&mapImg, m_sigValues[0], lastPnt, endPoint, dx,dy);
	
	mapImg.save("mapImg.jpg");
	
	m_sigMapItem->setPixmap(QPixmap::fromImage(mapImg.scaled(origSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
}

QColor MapGraphicsScene::colorForSignal(double sig)
{
	int colorIdx = (int)(sig * 100);
	if(colorIdx > 99)
		colorIdx = 99;
	if(colorIdx < 0)
		colorIdx = 0;
	
	return m_colorList[colorIdx];
}

void MapGraphicsScene::renderTriangle(QImage *img, SigMapValue *a, SigMapValue *b, SigMapValue *c, double dx, double dy)
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
		<< QPoint(a->point.x() * dx, a->point.y() * dy)
		<< QPoint(b->point.x() * dx, b->point.y() * dy)
		<< QPoint(c->point.x() * dx, c->point.y() * dy);
		
	QList<QColor> colors = QList<QColor>()
		<< colorForSignal(a->value)
		<< colorForSignal(b->value)
		<< colorForSignal(c->value);

	qDebug() << "MapGraphicsScene::renderTriangle: "<<points;
	if(!fillTriColor(img, points, colors))
		qDebug() << "MapGraphicsScene::renderTriangle: Not rendered";
		
// 	QPainter p(img);
// 	//p.setBrush(colors[0]);
// 	p.setPen(QPen(colors[2],3.0));
// 	p.drawConvexPolygon(points);
// 	p.end();

}
