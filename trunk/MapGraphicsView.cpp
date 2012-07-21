#include "MapGraphicsView.h"
#include "MapGraphicsScene.h"

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
