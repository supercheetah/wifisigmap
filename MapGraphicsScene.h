#ifndef MapGraphicsScene_H
#define MapGraphicsScene_H

#include <QtGui>

#ifdef Q_OS_ANDROID
#include <QGeoPositionInfoSource>
#endif

#include "WifiDataCollector.h"

class MapGraphicsView : public QGraphicsView
{
	Q_OBJECT
public:
	MapGraphicsView();
	void scaleView(qreal scaleFactor);
	
	double scaleFactor() { return m_scaleFactor; } 

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
};

class SigMapValue;
class SigMapValueMarker : public QObject, public QGraphicsPixmapItem
{
	Q_OBJECT
public:
	SigMapValueMarker(SigMapValue *val, const QPixmap& pixmap = QPixmap(), QGraphicsItem *parent = 0)
		: QGraphicsPixmapItem(pixmap, parent)
		, m_value(val) {}
	
	SigMapValue *value() { return m_value; }
	
signals:
	void clicked();
	
protected:
	virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * event )
	{
		QGraphicsItem::mouseReleaseEvent(event);
		
		emit clicked();
	}
	
	SigMapValue *m_value;
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
	
	SigMapValueMarker *marker;
	
	
	bool hasAp(QString mac);
	double signalForAp(QString mac, bool returnDbmValue=false);
};

class MapGraphicsScene;

class SigMapItem : public QGraphicsItem
{
public:
	SigMapItem();
	
	void setInternalCache(bool flag);
	void setPicture(QPicture pic);
	//void setPicture(QImage img);
	
	QRectF boundingRect() const;	
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
	bool m_internalCache;
	QPicture m_pic;
	QImage m_img;
	QPoint m_offset;
};


class LongPressSpinner : public QObject, public QGraphicsItem
{
	Q_OBJECT
public:
	LongPressSpinner();
	
	QRectF boundingRect() const;	
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public slots:
	void setVisible(bool flag);
	void setGoodPress(bool flag = true);
	void setProgress(double); // [0,1]

protected slots:
	void goodPressExpire();
	void fadeTick();

protected:
	bool m_goodPressFlag;
	double m_progress;
	QRectF m_boundingRect;
	QTimer m_goodPressTimer;

};

class SigMapRenderer : public QObject
{
	Q_OBJECT
public:
	SigMapRenderer(MapGraphicsScene* gs);

public slots:
	void render();

signals:
	void renderProgress(double); // 0.0-1.0
	void renderComplete(QPicture); //QImage);
	//void renderComplete(QImage);

protected:
	MapGraphicsScene *m_gs;
};

class MapApInfo
{
public:
	MapApInfo(WifiDataResult r = WifiDataResult())
		: mac(r.mac)
		, essid(r.essid)
		, marked(false)
		, point(QPointF())
		, color(QColor())
		, renderOnMap(true)
		{}
		
	QString mac;
	QString essid;
	bool    marked;
	QPointF point;
	QColor  color;
	bool    renderOnMap;
};

class MapRenderOptions
{
public:
	bool cacheMapRender;
	bool showReadingMarkers;
	bool multipleCircles;
	bool fillCircles;
	int radialCircleSteps;
	int radialLevelSteps;
	int radialAngleDiff;
	int radialLevelDiff;
	int radialLineWeight;
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
	
	QList<MapApInfo*> apInfo() { return m_apInfo.values(); }
	MapApInfo* apInfo(WifiDataResult);
	MapApInfo* apInfo(QString apMac);
	
public slots:
	void saveResults(QString filename);
	void loadResults(QString filename);
	void setBgFile(QString filename);
	void setMarkApMode(bool flag=true);
	
	void setRenderMode(RenderMode mode);
	
	void setRenderAp(QString apMac, bool flag=true);
	void setRenderAp(MapApInfo *ap, bool flag=true);
	
	void setRenderOpts(MapRenderOptions);
	
protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent * mouseEvent);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * mouseEvent);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * mouseEvent);
	
	friend class MapGraphicsView; // for access to:
	void invalidateLongPress();
	
private slots:
	void debugTest();

	#ifdef Q_OS_ANDROID
	#define QGeoPositionInfo QtMobility::QGeoPositionInfo
	void positionUpdated(QGeoPositionInfo);
	#endif
	
	void sensorReadingChanged();
	
	void longPressTimeout();
	void longPressCount();
	
	void renderProgress(double);
	void renderComplete(QPicture pic); //QImage mapImg);
	//void renderComplete(QImage mapImg);
	
	void scanFinished(QList<WifiDataResult>);
	
protected:
	friend class SigMapRenderer;
	
	void addSignalMarker(QPointF point, QList<WifiDataResult> results);
	QColor colorForSignal(double sig, QString apMac);
	QColor baseColorForAp(QString apMac);
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
 	SigMapItem *m_sigMapItem;
	//QGraphicsPixmapItem *m_sigMapItem;
	QString m_bgFilename;
	QPixmap m_bgPixmap;
	QGraphicsPixmapItem *m_bgPixmapItem;
	//QList<QColor> m_colorList;
	
	WifiDataCollector m_scanIf;
	
	int m_colorCounter;
	QList<QColor> m_masterColorsAvailable;
	QHash<QString,QList<QColor > > m_colorListForMac;
	QList<qreal> m_huesUsed;
	
	
	QHash<QString,MapApInfo*> m_apInfo; 
	
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
	//QHash<QString,QGraphicsPixmapItem *> m_apGuessItems;
	
	QList<WifiDataResult> m_lastScanResults;
	
	MapRenderOptions m_renderOpts;
	void showRenderOptsDialog();
	
	LongPressSpinner *m_longPressSpinner;
	
};

#endif
