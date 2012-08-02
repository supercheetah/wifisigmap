#include "MapGraphicsView.h"
#include "MapGraphicsScene.h"
#include "3rdparty/flickcharm.h"
#include "ImageUtils.h"

MapSignalHistory::MapSignalHistory(QColor c, int maxHistory)
	: color(c)
	, maxSize(maxHistory)
{
	if(maxSize < 0)
		maxSize = 10;
	
	for(int i=0; i<maxSize; i++)
		history << -1;
}

void MapSignalHistory::addValue(double v)
{
	while(history.size() > maxSize)
		history.takeFirst();
	
	history << v;
}

QImage MapSignalHistory::renderGraph(QSize size)
{
	QImage img(size, QImage::Format_ARGB32_Premultiplied);
	memset(img.bits(), 0, img.byteCount());
	QPainter p(&img);
	p.setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing);
	
	double height = (double)size.height() * 1.2; // HACK
	double width  = (double)size.width();
	double step   = width / (double)history.size();
	
	QPointF pntOne(1.,1.);
	QPointF lastPoint;
	for(int i=0; i<history.size(); i++)
	{
		double x = (double)i * step;
		double y = height - history[i] * height;
		
		QPointF pnt(x,y);
		if(lastPoint.isNull())
		{
			lastPoint = pnt;
			//qDebug() << "signalhist: started last pnt: "<<lastPoint;
		}
			
// 		p.setPen(QPen(color.lighter(500), 1.));
// 		p.drawLine(lastPoint-pntOne/2, pnt-pntOne/2);
		
		p.setPen(color.darker(500));
		p.drawLine(lastPoint/*+pntOne*/, pnt+pntOne);
		
		p.setPen(color.lighter());
		p.drawLine(lastPoint, pnt);
		
		lastPoint = pnt;
	}
	p.end();
	return img;
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
	//setTransformationAnchor(AnchorUnderMouse);
#ifdef Q_OS_ANDROID
	setTransformationAnchor(QGraphicsView::AnchorViewCenter);
#endif
	setResizeAnchor(AnchorViewCenter);
	setDragMode(QGraphicsView::ScrollHandDrag);

	setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform );
	// if there are ever graphic glitches to be found, remove this again
	setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing | QGraphicsView::DontClipPainter | QGraphicsView::DontSavePainterState);

	//setCacheMode(QGraphicsView::CacheBackground);
	//setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
	setOptimizationFlags(QGraphicsView::DontSavePainterState);

	setFrameStyle(QFrame::NoFrame);
	
	#ifdef Q_OS_ANDROID
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	#endif
	
	
	QWidget *viewport = new QWidget();
	QVBoxLayout *vbox = new QVBoxLayout(viewport);
	QHBoxLayout *hbox;
 	
 	hbox = new QHBoxLayout();
 	{
 		hbox->setSpacing(0);
 		hbox->addStretch(1);
 		
 		m_hudLabel = new QLabel();
 		hbox->addWidget(m_hudLabel);
 		
 		vbox->addLayout(hbox);
 	}
 	
	vbox->addStretch(1);

	hbox = new QHBoxLayout();
	{
		hbox->setSpacing(0);
		hbox->addStretch(1);

		m_statusLabel = new QLabel();
		hbox->addWidget(m_statusLabel);

		m_statusLabel->hide();

		hbox->addStretch(1);
	}

	vbox->addLayout(hbox);
	
	hbox = new QHBoxLayout();
	{
		hbox->setSpacing(0);
		hbox->addStretch(1);
		
		QPushButton *btn;
		{
			btn = new QPushButton("-");
			btn->setStyleSheet("QPushButton {"
				#ifdef Q_OS_ANDROID
				"background: url(':/data/images/android-zoom-minus-button.png'); width: 117px; height: 47px; "
				#else
				"background: url(':/data/images/zoom-minus-button.png'); width: 58px; height: 24px; "
				#endif
				"padding:0; margin:0; border:none; color: transparent; outline: none; }");
			btn->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
			btn->setAutoRepeat(true);
			btn->setMaximumSize(117,48);

			connect(btn, SIGNAL(clicked()), this, SLOT(zoomOut()));
			hbox->addWidget(btn);
		}
		
		{
			btn = new QPushButton("+");
			btn->setStyleSheet("QPushButton {"
				#ifdef Q_OS_ANDROID
				"background: url(':/data/images/android-zoom-plus-button.png'); width: 117px; height: 47px; "
				#else
				"background: url(':/data/images/zoom-plus-button.png'); width: 58px; height: 24px; "
				#endif
				"padding:0; margin:0; border:none; color: transparent; outline: none; }");
			btn->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
			btn->setAttribute(Qt::WA_TranslucentBackground, true);
			btn->setAutoRepeat(true);
			btn->setMaximumSize(117,48);
			
			connect(btn, SIGNAL(clicked()), this, SLOT(zoomIn()));
			hbox->addWidget(btn);
		}
		
		hbox->addStretch(1);
	}
	
	vbox->addLayout(hbox);
	
	m_viewportLayout = vbox;
	setViewport(viewport);
	
	// Set a timer to update layout because it doesn't give buttons correct position right at the start
	QTimer::singleShot(500, this, SLOT(updateViewportLayout()));
	
	// Disable here because it interferes with the 'longpress' functionality in MapGraphicsScene
// 	FlickCharm *flickCharm = new FlickCharm(this);
// 	flickCharm->activateOn(this);
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
	
	updateViewportLayout();
}

void MapGraphicsView::showEvent(QShowEvent *)
{
	updateViewportLayout();
}


void MapGraphicsView::mouseMoveEvent(QMouseEvent * mouseEvent)
{
	qobject_cast<MapGraphicsScene*>(scene())->invalidateLongPress(mapToScene(mouseEvent->pos()));

	updateViewportLayout();
	
	QGraphicsView::mouseMoveEvent(mouseEvent);
}

void MapGraphicsView::updateViewportLayout()
{
	m_viewportLayout->update();
}

void MapGraphicsView::setMapScene(MapGraphicsScene *scene)
{
	setScene(scene);
	m_gs = scene;
	
	connect(m_gs, SIGNAL(scanResultsAvailable(QList<WifiDataResult>)), this, SLOT(scanFinished(QList<WifiDataResult>)));
}

void MapGraphicsView::scanFinished(QList<WifiDataResult> results)
{
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
	
	int numLines = results.size();
	if(numLines <= 0)
		numLines = 1; // for a special message
		
	int imgWidth = 150;
	int imgHeight = numLines * lineJump + margin * 3;
	
	QImage img(imgWidth, imgHeight, QImage::Format_ARGB32_Premultiplied);
	memset(img.bits(), 0, img.byteCount()); 
	QPainter p(&img);
	
	p.fillRect(img.rect(), QColor(0,0,0,150));
	
	p.setFont(QFont("Monospace", fontSize, QFont::Bold));
	
	QSize historySize(
		//(int)(imgWidth - margin * 2),
		margin * 4,
		(int)(lineJump - 4.));

	foreach(WifiDataResult result, results)
	{
		if(!m_apSigHist.contains(result.mac))
			m_apSigHist.insert(result.mac, new MapSignalHistory(m_gs->baseColorForAp(result.mac)));
		
		MapSignalHistory *hist = m_apSigHist.value(result.mac);
		hist->addValue(result.value);
		
		QColor color = m_gs->colorForSignal(1.0, result.mac).lighter(100);
#ifdef Q_OS_ANDROID
		color = color.darker(300);
#endif
		QColor outline = Qt::white; //qGray(color.rgb()) < 60 ? Qt::white : Qt::black;
		QPoint pnt(QPoint(
			margin/2 + historySize.width(),
			y += lineJump));
		qDrawTextC(p, pnt.x(), pnt.y(),
			QString( "%1% %2"  )
				.arg(QString().sprintf("%02d", (int)round(result.value * 100.)))
				.arg(result.essid),
			outline,
			color);
			
		p.setOpacity(0.80);
		p.drawImage(
			QPoint(
				0 /*margin*/, 
				(int)(y - lineJump * .75)
			),
			hist->renderGraph(historySize)
				.mirrored(true,false)
		);
		
		p.setOpacity(1.0);
	}
	
	if(results.size() <= 0)
	{
		qDrawTextC(p, margin, margin + margin + p.font().pointSize() * 2, tr("No APs nearby or WiFi off"), Qt::red, Qt::white);
	}
	
	// Draw border on top of any text that may overflow
	p.setPen(Qt::black);
	p.drawRect(img.rect().adjusted(0,0,-1,-1));
	
// 	qDebug() << "MapGraphicsView::scanFinished(): Rendered "<<img.size()<<" size HUD";
// 	img.save("hud.png");
	
	p.end();
	m_hudLabel->setPixmap(QPixmap::fromImage(img));
	//updateViewportLayout();
	
}

void MapGraphicsView::setStatusMessage(QString msg)
{
	if(msg.isEmpty())
	{
		m_statusLabel->setPixmap(QPixmap());
		m_statusLabel->hide();
		return;
	}

	m_statusLabel->show();

	if(Qt::mightBeRichText(msg))
	{
		QTextDocument doc;
		doc.setHtml(msg);
		msg = doc.toPlainText();
	}
	
	QImage tmp(1,1,QImage::Format_ARGB32_Premultiplied);
	QPainter tmpPainter(&tmp);

	QFont font("");//"",  20);
	tmpPainter.setFont(font);
	
	QRectF maxRect(0, 0, (qreal)width(), (qreal)height() * .25);
	QRectF boundingRect = tmpPainter.boundingRect(maxRect, Qt::TextWordWrap | Qt::AlignHCenter, msg);
	boundingRect.adjust(0, 0, tmpPainter.font().pointSizeF() * 3, tmpPainter.font().pointSizeF() * 1.25);

	QImage labelImage(boundingRect.size().toSize(), QImage::Format_ARGB32_Premultiplied);
	memset(labelImage.bits(), 0, labelImage.byteCount());
	
	QPainter p(&labelImage);

	QColor bgColor(0, 127, 254, 180);

#ifdef Q_OS_ANDROID
	bgColor = bgColor.lighter(300);
#endif
	
	p.setPen(QPen(Qt::white, 2.5));
	p.setBrush(bgColor);
	p.drawRoundedRect(labelImage.rect().adjusted(0,0,-1,-1), 3., 3.);

	QImage txtImage(boundingRect.size().toSize(), QImage::Format_ARGB32_Premultiplied);
	memset(txtImage.bits(), 0, txtImage.byteCount());
	QPainter tp(&txtImage);
	
	tp.setPen(Qt::white);
	tp.setFont(font);
	tp.drawText(QRectF(QPointF(0,0), boundingRect.size()), Qt::TextWordWrap | Qt::AlignHCenter | Qt::AlignVCenter, msg);
	tp.end();

	double ss = 8.;
	p.drawImage((int)-ss,(int)-ss, ImageUtils::addDropShadow(txtImage, ss));
	
	p.end();

#ifdef Q_OS_ANDROID
	m_statusLabel->setPixmap(QPixmap::fromImage(labelImage));
#else
	m_statusLabel->setPixmap(QPixmap::fromImage(ImageUtils::addDropShadow(labelImage, ss)));
#endif
}

/// End MapGraphicsView implementation
