#ifndef MapGraphicsScene_H
#define MapGraphicsScene_H

#include <QtGui>

#ifdef Q_OS_ANDROID
//#include <QGeoPositionInfoSource>
#endif

#ifdef OPENCV_ENABLED
#include "KalmanFilter.h"
#endif

#include "WifiDataCollector.h"

#ifdef Q_OS_ANDROID
#define AndroidDialogHelper(widgetPtr, dialog) { \
	if(widgetPtr) \
	{	dialog.show(); \
		dialog.move(mapToGlobal(QPoint(widgetPtr->width()/2  - dialog.width()/2, widgetPtr->height()/2 - dialog.height()/2 ))); \
	} \
}
#else
#define AndroidDialogHelper(widgetPtr, dialog) { Q_UNUSED(widgetPtr); Q_UNUSED(dialog); }
#endif

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
		, rxGain(
		#ifdef Q_OS_ANDROID
			-3
		#else
			+3
		#endif
		)
		, rxMac("")
		, timestamp(QDateTime::currentDateTime())
		, lat(0.)
		, lng(0.)
		, scanResults(results)
		// For rendering and internal use, not stored in datafile:
 		, renderDataDirty(true)
 		, renderCircleRadius(0.)
 		, renderLevel(0.)
 		, renderAngle(0.)
		, consumed(false)
		, marker(0)
 		{}
	
	// Location on the map
	QPointF point;
	
	double rxGain; // dBi, typically in the range of [-3,+3] - +3 for laptops, -3 for phones, 0 for unknown or possibly tablets
	QString rxMac; // for estimating gain ...?
	QDateTime timestamp;
	
	double lat; // future use
	double lng;
	
	// Actual scan data for this point
	QList<WifiDataResult> scanResults;
	
	// Utils for accessing data in scanResults
	bool hasAp(QString mac);
	double signalForAp(QString mac, bool returnDbmValue=false);
	
	// Cache fields for rendering
	bool renderDataDirty;
	double renderCircleRadius;
	double renderLevel;
	double renderAngle;
	
	bool consumed;
	
	// The actual marker on the map
	SigMapValueMarker *marker;
	
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
	
	QPoint offset() { return m_offset; }
	QPicture picture() { return m_pic; }

protected:
	bool m_internalCache;
	QPicture m_pic;
	QImage m_img;
	QPoint m_offset;
};


class LongPressSpinner : public QObject, public QGraphicsItem
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)
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
		, mfg("")
		, txPower(11.9)
		, txGain(5.0)
		, lossFactor(2.,2.)
		, lossFactorKey(0) // init to 0, not -1, because -1 means user specified lossFactor,
		// but 0 means auto calculate (and store points used in lossFactoryKey to regenerate if # points changes)
		, shortCutoff(-49)
		{
			#ifdef OPENCV_ENABLED
			kalman.predictionBegin(0,0);
			#endif
		}
		
	QString mac;
	QString essid;
	bool    marked;
	QPointF point;
	QColor  color;
	bool    renderOnMap;
	
	QString mfg; // Guessed based on MAC OUI
	
	double  txPower; // dBm, 11.9 dBm = 19 mW (approx) = Linksys WRT54 Default TX power (approx)
	double  txGain;  // dBi,  5.0 dBi = Linksys WRT54 Stock Antenna Gain (approx)
	QPointF lossFactor; // arbitrary tuning parameters X and Y, typically in the range of [-1.0,5.0]
			    // (formula variable 'n' for short and far dBms, respectively)
	int lossFactorKey; // A 'cache key' - when auto-deriving loss factor, this is set to the # of points used, or -1 if the user manually specifies loss factor (TODO) 
	int shortCutoff; // dBm (typically -49) value at which to swith from loss factor X to loss factor Y for calculating distance 
			 // (less than shortFactorCutoff [close to AP], use X, greater [farther from AP] - use Y)
	
	#ifdef OPENCV_ENABLED
	KalmanFilter kalman;
	#endif
	
	QPointF locationGuess;

	#ifdef OPENCV_ENABLED
	KalmanFilter locationGuessKalman;
	#endif

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
		RenderRectangles,
	};
	
	RenderMode renderMode() { return m_renderMode; }
	
	QList<MapApInfo*> apInfo() { return m_apInfo.values(); }
	MapApInfo* apInfo(WifiDataResult);
	MapApInfo* apInfo(QString apMac);
	
	double meterPx() { return m_meterPx; }
	bool showMyLocation() { return m_showMyLocation; }
	bool autoGuessApLocations() { return m_autoGuessApLocations; }
	
public slots:
	void saveResults(QString filename);
	void loadResults(QString filename);
	void setBgFile(QString filename);
	void setMarkApMode(bool flag=true);
	
	void setRenderMode(RenderMode mode);
	
	void setRenderAp(QString apMac, bool flag=true);
	void setRenderAp(MapApInfo *ap, bool flag=true);
	
	void setRenderOpts(MapRenderOptions);

	void setMeterPx(double meters);
	
	void setShowMyLocation(bool flag=true);
	void setAutoGuessApLocations(bool flag=true);
	
protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent * mouseEvent);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * mouseEvent);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * mouseEvent);
	
	friend class MapGraphicsView; // for access to:
	void invalidateLongPress(QPointF pnt = QPointF());
	
private slots:
	void debugTest();

	#ifdef Q_OS_ANDROID
	//void positionUpdated(QtMobility::QGeoPositionInfo);
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
	
	SigMapValue *addSignalMarker(QPointF point, QList<WifiDataResult> results);
	QColor colorForSignal(double sig, QString apMac);
	QColor baseColorForAp(QString apMac);
	void renderTriangle(QImage *img, SigMapValue *a, SigMapValue *b, SigMapValue *c, double dx, double dy, QString apMac);
	void renderTriangle(QImage *img, QPointF center, SigMapValue *b, SigMapValue *c, double dx, double dy, QString apMac);
	
	void addApMarker(QPointF location, QString mac);

	//SigMapValue *findNearest(SigMapValue *match, QString apMac);
	SigMapValue *findNearest(QPointF match, QString apMac);
	QImage addDropShadow(QImage markerGroup, double shadowSize=16.);
	
	double getRenderLevel(double level,double angle,QPointF realPoint,QString apMac,QPointF center,double circleRadius);
	
	/** \brief Calculate an approximate distance from the AP given the received signal strength in dBm
	 \a dBm - received signal strength in dBm
	 \a lossFactor - loss factor describing obstacles, walls, reflection, etc. Can derive existing factor with driveObservedLossFactor()
	 \a shortCutoff - dBm at which to switch from the long lossFactor (x) to the short lossFactor (y) for readings close to the AP
	 \a txPower - dBm transmit power of the AP
	 \a txGain - dBi gain of the AP antennas
	 \a rxGain - dBi gain of the receiving antenna
	 \return Distance in meters from the AP
	*/
	double dBmToDistance(int dBm, QPointF lossFactor=QPointF(2.,2.), int shortCutoff=-49, double txPower=11.9, double txGain=5.0,  double rxGain=3.);
	
	/** \brief dBmToDistance(dBm, apMac, rxGain) is just a shortcut to dBmToDistance(), retrieves lossFactor, shortCutoff, txPower, txGain from stored values in apInfo(apMac)
	*/ 
	double dBmToDistance(int dBm, QString apMac,   double rxGain=3.);
	
	/** \brief deriveObservedLossFactor() calculates an approx lossFactor from observed signal readings for both short and long factors (x and y)
	    NOTE: Assumes m_meterPx is set correctly to convert pixel distance on the current background (map) to meters
	*/
	QPointF deriveObservedLossFactor(QString apMac);

	/** \brief deriveImpliedLossFactor() calculates an approx lossFactor from observed signal readings *based on location of readings* (it assumes AP locations are unknown) for both short and long factors (x and y)
	    NOTE: Assumes m_meterPx is set correctly to convert pixel distance on the current background (map) to meters
	*/
	QPointF deriveImpliedLossFactor(QString apMac);
	

	/// \brief Calculate the appropriate loss factor which would give distMeters
	double deriveLossFactor(QString apMac, int dBm, double distMeters, double rxGain=3.);
	
	QPointF triangulate(QString apMac1, int dBm1, QString apMac2, int dBm2);
	QPointF triangulateAp(QString apMac, SigMapValue *val0, SigMapValue *val1);
	
	
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
	
	LongPressSpinner *m_longPressSpinner;
	
	/// TODO make user configurable
	double m_footPx;
	double m_meterPx;
	
	// On first render complete after load file, reset scrollbars and zoom
	bool m_firstRender;
	int m_scrollHPos;
	int m_scrollVPos;
	double m_viewScale;
	
	#ifdef OPENCV_ENABLED
	KalmanFilter m_kalman;
	#endif
	
	bool m_showMyLocation;
	bool m_autoGuessApLocations;
};

#endif
