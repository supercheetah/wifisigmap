#ifndef MapGraphicsScene_H
#define MapGraphicsScene_H

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
 		// For rendering and internal use, not stored in datafile:
 		, renderDataDirty(true)
 		, renderCircleRadius(0.)
 		, renderLevel(0.)
 		, renderAngle(0.)
		{}
		
	QPointF point;
	bool consumed;
	
	QList<WifiDataResult> scanResults;
	
	bool renderDataDirty;
	double renderCircleRadius;
	double renderLevel;
	double renderAngle;
	
	
	bool hasAp(QString mac);
	double signalForAp(QString mac, bool returnDbmValue=false);
};

class MapGraphicsScene;

class SigMapRenderer : public QObject
{
	Q_OBJECT
public:
	SigMapRenderer(MapGraphicsScene* gs);

public slots:
	void render();

signals:
	void renderProgress(double); // 0.0-1.0
	void renderComplete(QImage);

protected:
	MapGraphicsScene *m_gs;
};

class MapWindow;

class MapGraphicsScene : public QGraphicsScene
{
	Q_OBJECT
public:
	MapGraphicsScene(MapWindow *mapWindow);
	~MapGraphicsScene();
	
	QString currentMapFilename() { return m_currentMapFilename; }
	QString currentBgFilename() { return m_bgFilename; }
	
	void clear();
	
	enum RenderMode {
		RenderRadial,
		RenderCircles,
		RenderTriangles,
	};
	
	RenderMode renderMode() { return m_renderMode; }
	
public slots:
	void saveResults(QString filename);
	void loadResults(QString filename);
	void setBgFile(QString filename);
	void setMarkApMode(bool flag=true);
	
	void setRenderMode(RenderMode mode);
	
protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent * mouseEvent);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * mouseEvent);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * mouseEvent);
	
	friend class MapGraphicsView; // for access to:
	void invalidateLongPress();
	
private slots:
	void longPressTimeout();
	void longPressCount();
	
	void renderProgress(double);
	void renderComplete(QImage mapImg);
	
	void scanFinished(QList<WifiDataResult>);
	
protected:
	friend class SigMapRenderer;
	
	void addSignalMarker(QPointF point, QList<WifiDataResult> results);
	QColor colorForSignal(double sig, QString apMac);
	void renderTriangle(QImage *img, SigMapValue *a, SigMapValue *b, SigMapValue *c, double dx, double dy, QString apMac);
	void renderTriangle(QImage *img, QPointF center, SigMapValue *b, SigMapValue *c, double dx, double dy, QString apMac);
	
	void addApMarker(QPointF location, QString mac);

	//SigMapValue *findNearest(SigMapValue *match, QString apMac);
	SigMapValue *findNearest(QPointF match, QString apMac);
	QImage addDropShadow(QImage markerGroup, double shadowSize=16.);
	
	double getRenderLevel(double level,double angle,QPointF realPoint,QString apMac,QPointF center,double circleRadius);
	
protected:
	QPointF m_pressPnt;
	QTimer m_longPressTimer;
	QTimer m_longPressCountTimer;
	int m_longPressCount;
	
	QList<SigMapValue*> m_sigValues;
		
	void addSigMapItem();
	QGraphicsPixmapItem *m_sigMapItem;
	QString m_bgFilename;
	QPixmap m_bgPixmap;
	QGraphicsPixmapItem *m_bgPixmapItem;
	//QList<QColor> m_colorList;
	
	WifiDataCollector m_scanIf;
	
	int m_colorCounter;
	QList<QColor> m_masterColorsAvailable;
	QHash<QString,QColor> m_apMasterColor;
	QHash<QString,QList<QColor > > m_colorListForMac;
	QList<qreal> m_huesUsed;
	
	QHash<QString,QPointF> m_apLocations;
	
	bool m_markApMode;
	
	bool m_develTestMode;
	
	QString m_currentMapFilename;
	
	MapWindow *m_mapWindow;
	
	RenderMode m_renderMode;
	
	SigMapRenderer *m_renderer;
	QThread m_renderingThread;
	QTimer m_renderTrigger;
	void triggerRender();
	
	QGraphicsPixmapItem *m_userItem;
	QHash<QString,QGraphicsPixmapItem *> m_apGuessItems;
};

#endif
