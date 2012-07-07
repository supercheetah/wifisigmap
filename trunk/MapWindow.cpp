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
	
	QSizeF sz = m_bgPixmap.size();
	double w = sz.width();
	double h = sz.height();
	
	// TODO just for debugging - TODO add in mode so user can specify AP locations
	m_apLocations["TE:ST:MA:C1:23:00"] = QPointF(w*.1, h*.8);
	m_apLocations["TE:ST:MA:C1:23:01"] = QPointF(w*.6, h*.6);
	m_apLocations["TE:ST:MA:C1:23:02"] = QPointF(w*.8, h*.1);
	
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
	/// This is just development code - need to integrate with WifiDataCollector
// 	bool ok;
// 	int i = QInputDialog::getInt(0, tr("Input Strength"),
// 					tr("Strength (%):"), 25, 0, 100, 1, &ok);
// 	if(ok)
// 	{
// 		addSignalMarker(m_pressPnt, ((double)i)/100.);
// 		QTimer::singleShot(0, this, SLOT(renderSigMap()));
// 	}

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
		
		double fakeSignalLevel = distFromCenter / (rangeMax * .35); // reduce for lower falloff
		
		const float range = 15.;
		float rv = ((float)(rand() % (int)range) - (range/2.)) / 100.; // Add a random +/- 5% to the value to prevent deadlocking
	
		fakeSignalLevel += rv; // add jitter
		
		fakeSignalLevel = 1 - fakeSignalLevel; // invert level so further away from center is lower signal
	
		qDebug() << "MapGraphicsScene::longPressTimeout(): AP#"<<i<<": "<<apMac<<": dist: "<<distFromCenter<<", rv:"<<rv<<", fakeSignalLevel:"<<fakeSignalLevel;
		
		if(fakeSignalLevel > .05)
		{
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
	
	// Add in drop shadow
	double shadowSize = (double)iconSize / 2.;
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
		
	markerGroup.save("markerGroupDebug.jpg");
	
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


SigMapValue *MapGraphicsScene::findNearest(SigMapValue *match, QString apMac)
{
	if(!match)
		return 0;
		
	SigMapValue *nearest = 0;
	double minDist = (double)INT_MAX;
	
	foreach(SigMapValue *val, m_sigValues)
	{
		if(val->consumed || !val->hasAp(apMac))
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

// static QString MapGraphicsScene_SigMapValue_apMac = "";
// bool MapGraphicsScene_SigMapValue_compare(SigMapValue *a, SigMapValue *b)
// {
// 	return (a && b) ? a->signalForAp(MapGraphicsScene_SigMapValue_apMac) < b->signalForAp(MapGraphicsScene_SigMapValue_apMac) : true;
// }


void MapGraphicsScene::renderSigMap()
{
	QSize origSize = m_bgPixmap.size();
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
	
// 	QRadialGradient rg(QPointF(iconSize/2,iconSize/2),iconSize);
// 			rg.setColorAt(0, centerColor/*.lighter(100)*/);
// 			rg.setColorAt(1, centerColor.darker(500));
// 			p.setPen(Qt::black);
// 			p.setBrush(QBrush(rg));
// 			

	foreach(QString apMac, apMacToEssid.keys())
	{
// 		MapGraphicsScene_SigMapValue_apMac = mac;
// 		qSort(m_sigValues.begin(), m_sigValues.end(), MapGraphicsScene_SigMapValue_compare);
		
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
	
	/*
	foreach(QString apMac, apMacToEssid.keys())
	{
		foreach(SigMapValue *val, m_sigValues)
			val->consumed = false;
		
		m_sigValues[0]->consumed = true; // center
		
		// Find first two closest points to center [0]
		// Make triangle
		// Find next cloest point
		// Make tri from last point, this point, center
		// Go on till all points done
		
		// Render first traingle
		SigMapValue *lastPnt = findNearest(m_sigValues[0], apMac);
		SigMapValue *nextPnt = findNearest(lastPnt, apMac);
		renderTriangle(&mapImg, m_sigValues[0], lastPnt, nextPnt, dx,dy, apMac);
		
		// Store this point to close the map 
		SigMapValue *endPoint = nextPnt;
		
		// Render all triangles inside
		while((nextPnt = findNearest(lastPnt, apMac)) != NULL)
		{
			renderTriangle(&mapImg, m_sigValues[0], lastPnt, nextPnt, dx,dy, apMac);
			lastPnt = nextPnt;
			//lastPnt = 0;//DEBUG
		}
		
		// Close the map
		renderTriangle(&mapImg, m_sigValues[0], lastPnt, endPoint, dx,dy, apMac);
	}
	*/
		
	mapImg.save("mapImg.jpg");
	
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
		sigPainter.setCompositionMode(QPainter::CompositionMode_HardLight);
		sigPainter.setCompositionMode(QPainter::CompositionMode_Difference);
		sigPainter.fillRect( signalLevelImage.rect(), baseColor ); 
		
		sigPainter.end();
		
		// just for debugging (that's why the img is 10px high - so it is easier to see in this output)
		signalLevelImage.save(tr("signalLevelImage-%1.png").arg(apMac));
		
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

	qDebug() << "MapGraphicsScene::renderTriangle: "<<apMac<<": "<<points;
	if(!fillTriColor(img, points, colors))
		qDebug() << "MapGraphicsScene::renderTriangle: "<<apMac<<": Not rendered";
		
// 	QPainter p(img);
// 	//p.setBrush(colors[0]);
// 	p.setPen(QPen(colors[2],3.0));
// 	p.drawConvexPolygon(points);
// 	p.end();

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




