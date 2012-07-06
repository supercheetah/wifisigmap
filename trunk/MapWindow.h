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
	SigMapValue(QPointF p, QList<WifiDataResult> results)
		: point(p)
 		, scanResults(results)
		, consumed(false)
		 {}
		
	QPointF point;
	//double value;
	
	QList<WifiDataResult> scanResults;
	
	bool hasAp(QString mac);
	double signalForAp(QString mac, bool returnDbmValue=false);
	
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
	
	void saveResults(QString filename);
	void loadResults(QString filename);
	
private slots:
	void longPressTimeout();
	void renderSigMap();
	
private:
	void addSignalMarker(QPointF point, QList<WifiDataResult> results);
	SigMapValue *findNearest(SigMapValue *, QString apMac);
	QColor colorForSignal(double sig, QString apMac);
	void renderTriangle(QImage *img, SigMapValue *a, SigMapValue *b, SigMapValue *c, double dx, double dy, QString apMac);

private:
	QPointF m_pressPnt;
	QTimer m_longPressTimer;
	
	QList<SigMapValue*> m_sigValues;
		
	QGraphicsPixmapItem *m_sigMapItem;
	QPixmap m_bgPixmap;
	//QList<QColor> m_colorList;
	
	WifiDataCollector m_scanIf;
	
	QHash<QString,QList<QColor > > m_colorListForMac;
	QList<qreal> m_huesUsed;
	
	QHash<QString,QPointF> m_apLocations;
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
