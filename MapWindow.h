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

public slots:
	void zoomIn();
	void zoomOut();
	
protected:
	void keyPressEvent(QKeyEvent *event);
	void mouseMoveEvent(QMouseEvent * mouseEvent);
	void wheelEvent(QWheelEvent *event);
	//void drawBackground(QPainter *painter, const QRectF &rect);
};

class SigMapValue
{
public:
	SigMapValue(QPointF p, QList<WifiDataResult> results)
		: point(p)
		, consumed(false)
 		, scanResults(results)
		{}
		
	QPointF point;
	bool consumed;
	
	QList<WifiDataResult> scanResults;
	
	bool hasAp(QString mac);
	double signalForAp(QString mac, bool returnDbmValue=false);
};

class MapWindow;

class MapGraphicsScene : public QGraphicsScene
{
	Q_OBJECT
public:
	MapGraphicsScene(MapWindow *mapWindow);
	
	QString currentMapFilename() { return m_currentMapFilename; }
	QString currentBgFilename() { return m_bgFilename; }
	
	void clear();
	
public slots:
	void saveResults(QString filename);
	void loadResults(QString filename);
	void setBgFile(QString filename);
	void setMarkApMode(bool flag=true);
	
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
	void addSignalMarker(QPointF point, QList<WifiDataResult> results);
	QColor colorForSignal(double sig, QString apMac);
	void renderTriangle(QImage *img, SigMapValue *a, SigMapValue *b, SigMapValue *c, double dx, double dy, QString apMac);
	void renderTriangle(QImage *img, QPointF center, SigMapValue *b, SigMapValue *c, double dx, double dy, QString apMac);
	
	void addApMarker(QPointF location, QString mac);

	//SigMapValue *findNearest(SigMapValue *match, QString apMac);
	SigMapValue *findNearest(QPointF match, QString apMac);
	QImage addDropShadow(QImage markerGroup, double shadowSize=16.);
	
private:
	QPointF m_pressPnt;
	QTimer m_longPressTimer;
	
	QList<SigMapValue*> m_sigValues;
		
	void addSigMapItem();
	QGraphicsPixmapItem *m_sigMapItem;
	QString m_bgFilename;
	QPixmap m_bgPixmap;
	QGraphicsPixmapItem *m_bgPixmapItem;
	//QList<QColor> m_colorList;
	
	WifiDataCollector m_scanIf;
	
	QHash<QString,QList<QColor > > m_colorListForMac;
	QList<qreal> m_huesUsed;
	
	QHash<QString,QPointF> m_apLocations;
	
	bool m_markApMode;
	
	bool m_develTestMode;
	
	QString m_currentMapFilename;
	
	MapWindow *m_mapWindow;
};


class MapWindow : public QWidget
{
	Q_OBJECT
public:
	MapWindow(QWidget *parent=0);

protected:
	void setupUi();
	
	friend class MapGraphicsScene; // friend for access to the following (and flagApModeCleared())
	void setStatusMessage(const QString&, int timeout=-1);
	
protected slots:
	void saveSlot();
	void loadSlot();
	void chooseBgSlot();
	void clearSlot();
	
	void flagApModeCleared();
	void clearStatusMessage();

private:
	MapGraphicsView *m_gv;
	MapGraphicsScene *m_scene;
	QLabel *m_statusMsg;
	QPushButton *m_apButton;

	QTimer m_statusClearTimer;
};

#endif
