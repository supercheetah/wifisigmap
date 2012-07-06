#ifndef MapWindow_H
#define MapWindow_H

#include <QtGui>
#include "WifiDataCollector.h"

class MapGraphicsView : public QGraphicsView
{
	Q_OBJECT
public:
	MapGraphicsView();
	void scaleView(qreal scaleFactor);
	
protected:
	void mouseMoveEvent(QMouseEvent * mouseEvent);
	void wheelEvent(QWheelEvent *event);
	//void drawBackground(QPainter *painter, const QRectF &rect);
};

class SigMapValue
{
public:
	SigMapValue(QPointF p, double v)
		: point(p)
		, value(v)
		, consumed(false)
		 {}
		
	QPointF point;
	double value;
	
	bool consumed;
};

class MapGraphicsScene : public QGraphicsScene
{
	Q_OBJECT
public:
	MapGraphicsScene();
	
protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent * mouseEvent);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * mouseEvent);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * mouseEvent);
	
	friend class MapGraphicsView; // for access to:
	void invalidateLongPress();
	
private slots:
	void longPressTimeout();
	void renderSigMap();
	
private:
	void addSignalMarker(QPointF point, double value);
	SigMapValue *findNearest(SigMapValue *);
	QColor colorForSignal(double sig);
	void renderTriangle(QImage *img, SigMapValue *a, SigMapValue *b, SigMapValue *c, double dx, double dy);

private:
	QPointF m_pressPnt;
	QTimer m_longPressTimer;
	
	QList<SigMapValue*> m_sigValues;
		
	QGraphicsPixmapItem *m_sigMapItem;
	QPixmap m_bgPixmap;
	QList<QColor> m_colorList;
	
	WifiDataCollector m_scanIf;
};


class MapWindow : public QWidget
{
	Q_OBJECT
public:
	MapWindow(QWidget *parent=0);

protected:
	void setupUi();
	
private:
	MapGraphicsScene *m_scene;	

};

#endif
