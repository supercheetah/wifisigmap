#include "LongPressSpinner.h"

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
	fingerTipAdjust = (iconSize/2)/3;
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

