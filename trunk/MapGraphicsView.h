#ifndef MapGraphicsView_H
#define MapGraphicsView_H

#include <QtGui>
#include "WifiDataCollector.h"

class MapSignalHistory
{
public:
	MapSignalHistory(QColor color = Qt::white, int maxHistory=100);
	void addValue(double);
	QImage renderGraph(QSize);

private:
	QColor color;
	QList<double> history;
};

class MapGraphicsScene;
class MapGraphicsView : public QGraphicsView
{
	Q_OBJECT
public:
	MapGraphicsView();
	void scaleView(qreal scaleFactor);

	double scaleFactor() { return m_scaleFactor; }

	void reset();
	
	void setMapScene(MapGraphicsScene *);
	
public slots:
	void zoomIn();
	void zoomOut();
	
protected slots:
	void scanFinished(QList<WifiDataResult> results);
	void updateViewportLayout();

protected:
	void drawForeground(QPainter *p, const QRectF & rect);

	void keyPressEvent(QKeyEvent *event);
	void mouseMoveEvent(QMouseEvent * mouseEvent);
	void wheelEvent(QWheelEvent *event);
	void showEvent(QShowEvent *);
	//void drawBackground(QPainter *painter, const QRectF &rect);

	double m_scaleFactor;
	
	QHash<QString, MapSignalHistory*> m_apSigHist;
	
	QLayout *m_viewportLayout; 
	
	MapGraphicsScene *m_gs;
	QLabel *m_hudLabel;
};

#endif
