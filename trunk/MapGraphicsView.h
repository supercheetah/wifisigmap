#ifndef MapGraphicsView_H
#define MapGraphicsView_H

#include <QtGui>

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

class MapGraphicsView : public QGraphicsView
{
	Q_OBJECT
public:
	MapGraphicsView();
	void scaleView(qreal scaleFactor);

	double scaleFactor() { return m_scaleFactor; }

	void reset();

public slots:
	void zoomIn();
	void zoomOut();

protected:
	void drawForeground(QPainter *p, const QRectF & rect);

	void keyPressEvent(QKeyEvent *event);
	void mouseMoveEvent(QMouseEvent * mouseEvent);
	void wheelEvent(QWheelEvent *event);
	//void drawBackground(QPainter *painter, const QRectF &rect);

	double m_scaleFactor;
	
	QHash<QString, MapSignalHistory*> m_apSigHist;
};

#endif
