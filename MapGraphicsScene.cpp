#include "MapGraphicsScene.h"
#include "MapWindow.h"

#include "MapGraphicsView.h"
#include "LongPressSpinner.h"
#include "SigMapRenderer.h"
#include "ImageUtils.h"

#ifndef DEBUG
#ifdef Q_OS_ANDROID
#include <QSensor>
#include <QSensorReading>
#endif
#endif

#ifndef Q_OS_ANDROID
//#define ITEMS_SCALE_INVARIENT
#endif

///// Just for testing on linux, defined after DEBUG_WIFI_FILE so we still can use cached data
//#define Q_OS_ANDROID

#ifdef Q_OS_ANDROID
#define QT_NO_OPENGL
#endif

#ifndef QT_NO_OPENGL
#include <QtOpenGL/QGLWidget>
#endif

// #ifdef Q_OS_ANDROID
	#define DEFAULT_RENDER_MODE MapGraphicsScene::RenderCircles
//	#define DEFAULT_RENDER_MODE MapGraphicsScene::RenderRectangles
// #else
// 	#define DEFAULT_RENDER_MODE MapGraphicsScene::RenderRadial
// #endif

bool MapGraphicsScene_sort_WifiDataResult(WifiDataResult ra, WifiDataResult rb)
{
	return ra.value > rb.value;
}

/// Main class of this file - MapGraphicsScene

MapGraphicsScene::MapGraphicsScene(MapWindow *map)
	: QGraphicsScene()
	, m_colorCounter(0)
	, m_markApMode(false)
	, m_develTestMode(false)
	, m_mapWindow(map)
	, m_renderMode(DEFAULT_RENDER_MODE)
	, m_userItem(0)
	, m_apLocationOverlay(0)
	, m_debugFirstScan(true)
{
	m_renderUpdatesPaused = false;
	
	// Setup default render options
	m_renderOpts.cacheMapRender     = true;
	m_renderOpts.showReadingMarkers = true;
	m_renderOpts.multipleCircles	= false;
	m_renderOpts.fillCircles	= true;
	#ifdef Q_OS_ANDROID
	m_renderOpts.radialCircleSteps	= 4 * 4 * 2;
	m_renderOpts.radialLevelSteps	= (int)(100 / .2);
	#else
	m_renderOpts.radialCircleSteps	= 4 * 4 * 4;
	m_renderOpts.radialLevelSteps	= (int)(100 / .25);
	#endif
	m_renderOpts.radialAngleDiff	= 45 * 3;
	m_renderOpts.radialLevelDiff	= 100 / 3;
	m_renderOpts.radialLineWeight	= 200;

	// Create initial palette of colors for flagging APs
	m_masterColorsAvailable = QList<QColor>() 
		<< Qt::red 
		<< Qt::darkGreen 
		<< Qt::blue 
		<< Qt::magenta 
		<< "#0277fd" // cream blue 
		<< "#fd1402" // rich red
		<< "#f8fc0f" // kid yellow
		<< "#19f900" // kid green
		<< "#ff4500"  // dark orange
		;

	// Setup the "longpress" timer
	connect(&m_longPressTimer, SIGNAL(timeout()), this, SLOT(longPressTimeout()));
	m_longPressTimer.setInterval(1250);
	m_longPressTimer.setSingleShot(true);
	
	// Setup the longpress "progress counter" (display progress to user)
	connect(&m_longPressCountTimer, SIGNAL(timeout()), this, SLOT(longPressCount()));
	m_longPressCountTimer.setInterval(m_longPressTimer.interval() / 15);
	m_longPressCountTimer.setSingleShot(false);
	
	// Setup rendering thread
	//qDebug() << "MapGraphicsScene::MapGraphicsScene(): currentThreadId:"<<QThread::currentThreadId();
	m_renderer = new SigMapRenderer(this);
	m_renderer->moveToThread(&m_renderingThread);
	connect(m_renderer, SIGNAL(renderProgress(double)), this, SLOT(renderProgress(double)));
	connect(m_renderer, SIGNAL(renderComplete(QPicture)), this, SLOT(renderComplete(QPicture)));
	//connect(m_renderer, SIGNAL(renderComplete(QImage)), this, SLOT(renderComplete(QImage)));
	m_renderingThread.start();
	
	// Setup the trigger timer
	connect(&m_renderTrigger, SIGNAL(timeout()), m_renderer, SLOT(render()));
	m_renderTrigger.setInterval(50);
	m_renderTrigger.setSingleShot(true);
	
	// Setup the ap location update timer
	connect(&m_apGuessUpdateTimer, SIGNAL(timeout()), this, SLOT(updateApLocationOverlay()));
	m_apGuessUpdateTimer.setInterval(50);
	m_apGuessUpdateTimer.setSingleShot(true);
	
	// Delay rendering updates on signal markers so multiple calls in quick succession to setRenderAp() only triggers one rendering pass
	connect(&m_updateSignalMarkerTimer, SIGNAL(timeout()), this, SLOT(updateSignalMarkers()));
	m_updateSignalMarkerTimer.setInterval(50);
	m_updateSignalMarkerTimer.setSingleShot(true);
	
	// Received every time a scan is completed in continous mode (continuous mode being the default)
	connect(&m_scanIf, SIGNAL(scanFinished(QList<WifiDataResult>)), this, SLOT(scanFinished(QList<WifiDataResult>)));
	
	// Set up background and other misc items
	clear();
	
	// Load basic settings
	QSettings settings("wifisigmap");
	m_showMyLocation       = settings.value("showMyLocation", false).toBool();
	m_autoGuessApLocations = settings.value("autoGuessApLocations", false).toBool();

	// Set defualt pixels per foot
	setPixelsPerFoot(0.25/10);

	// Load last device/file used
	setDevice(settings.value("device", "").toString());

	// Load last render mode
	// Ignore settings JUST for DEBUGGING
#ifdef Q_OS_ANDROID
	m_renderMode = (RenderMode)0; //settings.value("rendermode", (int)DEFAULT_RENDER_MODE).toInt();
#else
	m_renderMode = (RenderMode)settings.value("rendermode", (int)DEFAULT_RENDER_MODE).toInt();
#endif

	// Ask MapRenderOpts to load itself from QSettings
	m_renderOpts.loadFromQSettings();
	
	qDebug() << "MapGraphicsScene: Setup and ready to go.";
	
	//QTimer::singleShot(1000, this, SLOT(debugTest()));
	QTimer::singleShot(500, this, SLOT(testUserLocatorAccuracy()));
	
	#ifdef OPENCV_ENABLED
	m_kalman.predictionBegin(0,0);
	#endif
}

MapGraphicsScene::~MapGraphicsScene()
{
	if(m_renderer)
	{
		m_renderingThread.quit();
		delete m_renderer;
		m_renderer = 0;
	}
}

void MapGraphicsScene::setDevice(QString dev)
{
	if(dev.isEmpty())
		dev = m_scanIf.findWlanIf();
	
	m_device = dev;

	QSettings("wifisigmap").setValue("device", dev);

	m_debugFirstScan = true;
	
	if(dev.contains("/") || dev.contains("\\")) // HACK to see if it's a file
	{
		m_scanIf.setWlanDevice("");
		m_scanIf.setDataTextfile(dev);
	}
	else
	{
		m_scanIf.setWlanDevice(dev);
		m_scanIf.setDataTextfile("");
	}
}

void MapGraphicsScene::debugTest()
{
	// Disable sensor testing for now - I'll work more on it later.
	return;
	#ifndef DEBUG
	#ifdef Q_OS_ANDROID
	QList<QByteArray> sensorList = QtMobility::QSensor::sensorTypes();
	qDebug() << "Sensor list length: "<<sensorList.size();
	foreach (QByteArray sensorName, sensorList)
	{
		qDebug() << "Sensor: "<<sensorName;
		QtMobility::QSensor *sensor = new QtMobility::QSensor(sensorName, this); // destroyed when this is destroyed
		sensor->setObjectName(sensorName);
		bool started = sensor->start();
		if(started)
		{
			qDebug() << " - Started!";
			connect(sensor, SIGNAL(readingChanged()), this, SLOT(sensorReadingChanged()));
		}
		else
			qDebug() << " ! Error starting";
	}

	/*
	// start the sensor
	QSensor sensor("QAccelerometer");
	sensor.start();

	// later
	QSensorReading *reading = sensor.reading();
	qreal x = reading->property("x").value<qreal>();
	qreal y = reading->value(1).value<qreal>();

	*/

	/// NOTE Starting QGeoPositionInfoSource causes the app to crash as of 2012-07-13
	/// Still crrashes as of 7/15
	/*
	QtMobility::QGeoPositionInfoSource *source = QtMobility::QGeoPositionInfoSource::createDefaultSource(this);
	if (source) {

	    //connect(source, SIGNAL(positionUpdated(QGeoPositionInfo)),
	    //	    this, SLOT(positionUpdated(QGeoPositionInfo)));

	    source->startUpdates();
	    qDebug() << "Started geo source:"<<source;
	    QtMobility::QGeoPositionInfo last = source->lastKnownPosition();
	    qDebug() << "Last position:" << last;
	}
	else
	    qDebug() << "No geo source found";
	*/

	#endif
	#endif
}



/** Helper function to compute the angle change between two rotation matrices.
*  Given a current rotation matrix (R) and a previous rotation matrix
*  (prevR) computes the rotation around the x,y, and z axes which
*  transforms prevR to R.
*  outputs a 3 element vector containing the x,y, and z angle
*  change at indexes 0, 1, and 2 respectively.
* <p> Each input matrix is either as a 3x3 or 4x4 row-major matrix
* depending on the length of the passed array:
* <p>If the array length is 9, then the array elements represent this matrix
* <pre>
*   /  R[ 0]   R[ 1]   R[ 2]   \
*   |  R[ 3]   R[ 4]   R[ 5]   |
*   \  R[ 6]   R[ 7]   R[ 8]   /
*</pre>
* <p>If the array length is 16, then the array elements represent this matrix
* <pre>
*   /  R[ 0]   R[ 1]   R[ 2]   R[ 3]  \
*   |  R[ 4]   R[ 5]   R[ 6]   R[ 7]  |
*   |  R[ 8]   R[ 9]   R[10]   R[11]  |
*   \  R[12]   R[13]   R[14]   R[15]  /
*</pre>
* @param R current rotation matrix
* @param prevR previous rotation matrix
* @param angleChange an array of floats in which the angle change is stored
*/

QVector<float> getAngleChange(QVector<float> R, QVector<float> prevR) 
{
	QVector<float> angleChange;
	float rd1=0,rd4=0, rd6=0,rd7=0, rd8=0;
	float ri0=0,ri1=0,ri2=0,ri3=0,ri4=0,ri5=0,ri6=0,ri7=0,ri8=0;
	float pri0=0, pri1=0, pri2=0, pri3=0, pri4=0, pri5=0, pri6=0, pri7=0, pri8=0;
	//int i, j, k;

	if(R.size() == 9)
	{
		ri0 = R[0];
		ri1 = R[1];
		ri2 = R[2];
		ri3 = R[3];
		ri4 = R[4];
		ri5 = R[5];
		ri6 = R[6];
		ri7 = R[7];
		ri8 = R[8];
	}
	else if(R.size() == 16)
	{
		ri0 = R[0];
		ri1 = R[1];
		ri2 = R[2];
		ri3 = R[4];
		ri4 = R[5];
		ri5 = R[6];
		ri6 = R[8];
		ri7 = R[9];
		ri8 = R[10];
	}

	if(prevR.size() == 9)
	{
		pri0 = R[0];
		pri1 = R[1];
		pri2 = R[2];
		pri3 = R[3];
		pri4 = R[4];
		pri5 = R[5];
		pri6 = R[6];
		pri7 = R[7];
		pri8 = R[8];
	}
	else if(prevR.size() == 16)
	{
		pri0 = R[0];
		pri1 = R[1];
		pri2 = R[2];
		pri3 = R[4];
		pri4 = R[5];
		pri5 = R[6];
		pri6 = R[8];
		pri7 = R[9];
		pri8 = R[10];
	}

	// calculate the parts of the rotation difference matrix we need
	// rd[i][j] = pri[0][i] * ri[0][j] + pri[1][i] * ri[1][j] + pri[2][i] * ri[2][j];

	rd1 = pri0 * ri1 + pri3 * ri4 + pri6 * ri7; //rd[0][1]
	rd4 = pri1 * ri1 + pri4 * ri4 + pri7 * ri7; //rd[1][1]
	rd6 = pri2 * ri0 + pri5 * ri3 + pri8 * ri6; //rd[2][0]
	rd7 = pri2 * ri1 + pri5 * ri4 + pri8 * ri7; //rd[2][1]
	rd8 = pri2 * ri2 + pri5 * ri5 + pri8 * ri8; //rd[2][2]

	angleChange.resize(3);
	angleChange[0] = (float)atan2(rd1, rd4);
	angleChange[1] = (float)asin(-rd7);
	angleChange[2] = (float)atan2(-rd6, rd8);
	
	return angleChange;
}

/** Helper function to convert a rotation vector to a rotation matrix.
*  Given a rotation vector (presumably from a ROTATION_VECTOR sensor), returns a
*  9  or 16 element rotation matrix. \a desiredMatrixSize must be 9 or 16.
*  If \a desiredMatrixSize == 9, the following matrix is returned:
* <pre>
*   /  R[ 0]   R[ 1]   R[ 2]   \
*   |  R[ 3]   R[ 4]   R[ 5]   |
*   \  R[ 6]   R[ 7]   R[ 8]   /
*</pre>
* If \a desiredMatrixSize == 16, the following matrix is returned:
* <pre>
*   /  R[ 0]   R[ 1]   R[ 2]   0  \
*   |  R[ 4]   R[ 5]   R[ 6]   0  |
*   |  R[ 8]   R[ 9]   R[10]   0  |
*   \  0       0       0       1  /
*</pre>
*  @param rotationVector the rotation vector to convert
*  @param R an array of floats in which to store the rotation matrix
*/
QVector<float> getRotationMatrixFromVector(QVector<float> rotationVector, int desiredMatrixSize = 9)
{
	QVector<float> R;
	R.resize(desiredMatrixSize);
	float q0 = (float)sqrt(1 - rotationVector[0]*rotationVector[0] -
				   rotationVector[1]*rotationVector[1] -
				   rotationVector[2]*rotationVector[2]);
	float q1 = rotationVector[0];
	float q2 = rotationVector[1];
	float q3 = rotationVector[2];

	float sq_q1 = 2 * q1 * q1;
	float sq_q2 = 2 * q2 * q2;
	float sq_q3 = 2 * q3 * q3;
	float q1_q2 = 2 * q1 * q2;
	float q3_q0 = 2 * q3 * q0;
	float q1_q3 = 2 * q1 * q3;
	float q2_q0 = 2 * q2 * q0;
	float q2_q3 = 2 * q2 * q3;
	float q1_q0 = 2 * q1 * q0;

	if(desiredMatrixSize == 9)
	{
		R[0] = 1 - sq_q2 - sq_q3;
		R[1] = q1_q2 - q3_q0;
		R[2] = q1_q3 + q2_q0;
	
		R[3] = q1_q2 + q3_q0;
		R[4] = 1 - sq_q1 - sq_q3;
		R[5] = q2_q3 - q1_q0;
	
		R[6] = q1_q3 - q2_q0;
		R[7] = q2_q3 + q1_q0;
		R[8] = 1 - sq_q1 - sq_q2;
	}
	else if (desiredMatrixSize == 16)
	{
		R[0] = 1 - sq_q2 - sq_q3;
		R[1] = q1_q2 - q3_q0;
		R[2] = q1_q3 + q2_q0;
		R[3] = 0.0f;
	
		R[4] = q1_q2 + q3_q0;
		R[5] = 1 - sq_q1 - sq_q3;
		R[6] = q2_q3 - q1_q0;
		R[7] = 0.0f;
	
		R[8] = q1_q3 - q2_q0;
		R[9] = q2_q3 + q1_q0;
		R[10] = 1 - sq_q1 - sq_q2;
		R[11] = 0.0f;
	
		R[12] = R[13] = R[14] = 0.0f;
		R[15] = 1.0f;
	}
	return R;
}

#if 0
    /** Helper function to convert a rotation vector to a normalized quaternion.
     *  Given a rotation vector (presumably from a ROTATION_VECTOR sensor), returns a normalized
     *  quaternion in the array Q.  The quaternion is stored as [w, x, y, z]
     *  @param rv the rotation vector to convert
     *  @param Q an array of floats in which to store the computed quaternion
     */
    public static void getQuaternionFromVector(float[] Q, float[] rv) {
	float w = (float)Math.sqrt(1 - rv[0]*rv[0] - rv[1]*rv[1] - rv[2]*rv[2]);
	//In this case, the w component of the quaternion is known to be a positive number

	Q[0] = w;
	Q[1] = rv[0];
	Q[2] = rv[1];
	Q[3] = rv[2];
    }

#endif

void MapGraphicsScene::sensorReadingChanged()
{
	#ifndef DEBUG
	#ifdef Q_OS_ANDROID
	QtMobility::QSensor *sensor = qobject_cast<QtMobility::QSensor*>(sender());
	const QtMobility::QSensorReading *reading = sensor->reading();
	QString sensorName(sensor->objectName());
	if(sensorName == "QAccelerometer")
	{
		// Gravity removal code from http://developer.android.com/reference/android/hardware/SensorEvent.html#values

		// alpha is calculated as t / (t + dT)
		// with t, the low-pass filter's time-constant
		// and dT, the event delivery rate

		static double gravity[3] = {1,1,1};

		const float alpha = 0.8;

		double values[3] = {
		    reading->value(0).toDouble(),
		    reading->value(1).toDouble(),
		    reading->value(2).toDouble()
		};

		gravity[0] = alpha * gravity[0] + (1 - alpha) * values[0];
		gravity[1] = alpha * gravity[1] + (1 - alpha) * values[1];
		gravity[2] = alpha * gravity[2] + (1 - alpha) * values[2];

		double linear_acceleration[3] = { 0,0,0 };
		linear_acceleration[0] = values[0] - gravity[0];
		linear_acceleration[1] = values[1] - gravity[1];
		linear_acceleration[2] = values[2] - gravity[2];

		//qDebug() << " --> Accelerometer: " << linear_acceleration[0] << linear_acceleration[1] << linear_acceleration[2];
	}
	else
	if(sensorName == "QGyroscope")
	{
		static const float NS2S = 1.0f / 1000000000.0f;
		static QVector<float> deltaRotationVector(4,0);
		static float timestamp = 0;

		double values[3] = {
		    reading->value(0).toDouble(),
		    reading->value(1).toDouble(),
		    reading->value(2).toDouble()
		};

		// This timestep's delta rotation to be multiplied by the current rotation
		// after computing it from the gyro sample data.
		if (timestamp != 0)
		{
			float dT = (reading->timestamp() - timestamp) * NS2S;

			// Axis of the rotation sample, not normalized yet.
			float axisX = values[0];
			float axisY = values[1];
			float axisZ = values[2];

			// Calculate the angular speed of the sample
			float omegaMagnitude = sqrt(axisX*axisX + axisY*axisY + axisZ*axisZ);

			// Normalize the rotation vector if it's big enough to get the axis
			if (omegaMagnitude > 0.0001) { // 1.0 was EPSILON
				axisX /= omegaMagnitude;
				axisY /= omegaMagnitude;
				axisZ /= omegaMagnitude;
			}

			// Integrate around this axis with the angular speed by the timestep
			// in order to get a delta rotation from this sample over the timestep
			// We will convert this axis-angle representation of the delta rotation
			// into a quaternion before turning it into the rotation matrix.
			float thetaOverTwo = omegaMagnitude * dT / 2.0f;
			float sinThetaOverTwo = sin(thetaOverTwo);
			float cosThetaOverTwo = cos(thetaOverTwo);
			deltaRotationVector[0] = sinThetaOverTwo * axisX;
			deltaRotationVector[1] = sinThetaOverTwo * axisY;
			deltaRotationVector[2] = sinThetaOverTwo * axisZ;
			deltaRotationVector[3] = cosThetaOverTwo;
		}
		timestamp = reading->timestamp();
		//float deltaRotationMatrix[] = { 0,0,0, 0,0,0, 0,0,0 };
		QVector<float> deltaRotationMatrix = getRotationMatrixFromVector(deltaRotationVector, 16);
		static QMatrix4x4 rotationCurrent;
		rotationCurrent *= QMatrix4x4((qreal*)deltaRotationMatrix.data());
		// User code should concatenate the delta rotation we computed with the current rotation
		// in order to get the updated rotation.
		//float rotationCurrent[] = { 1,1,1, 1,1,1, 1,1,1 };
		//rotationCurrent = rotationCurrent * deltaRotationMatrix;


	}
	else
	{
		/*
		int count = reading->valueCount();
		qDebug() << "Receved "<< count <<" value readings from: " << sensor->objectName();
		for(int i=0; i<count; i++)
			qDebug() << "\t Value " << i << ":" << reading->value(i);
		*/
	}
	#endif
	#endif
}

#ifdef Q_OS_ANDROID
/*
void MapGraphicsScene::positionUpdated(QtMobility::QGeoPositionInfo info)
{
	qDebug() << "Position updated:" << info;
}
*/
#endif

void MapGraphicsScene::triggerRender()
{
	if(m_renderUpdatesPaused)
		return;
		
	if(m_renderTrigger.isActive())
		m_renderTrigger.stop();
	m_renderTrigger.start(50);
	m_mapWindow->setStatusMessage("Rendering signal map...");
}

void MapGraphicsScene::setMarkApMode(bool flag)
{
	m_markApMode = flag;
	if(flag)
		m_mapWindow->setStatusMessage("Touch and hold map to mark AP location");
	else
		m_mapWindow->flagApModeCleared(); // resets push button state as well
}

void MapGraphicsScene::clear()
{
	m_currentMapFilename = "";

	qDeleteAll(m_apInfo.values());
	m_apInfo.clear();
	m_sigValues.clear();
	QGraphicsScene::clear();
	
	
	m_bgFilename = "";
	//m_bgPixmap = QPixmap(4000,4000); //QApplication::desktop()->screenGeometry().size());
	m_bgPixmap = QPixmap(2000,2000); //QApplication::desktop()->screenGeometry().size());
	m_bgPixmap.fill(Qt::white);
	
	QPainter p(&m_bgPixmap);
	p.setPen(QPen(Qt::gray,3.0));
	int fontSize = 10;
	int pad = fontSize/2;
#ifdef Q_OS_ANDROID
	fontSize = 5;
	pad = fontSize;
#endif
	p.setFont(QFont("Monospace", fontSize, QFont::Bold));
	
	for(int x=0; x<m_bgPixmap.width(); x+=64)
	{
		p.drawLine(x,0,x,m_bgPixmap.height());
		for(int y=0; y<m_bgPixmap.height(); y+=64)
		{
			p.drawText(x+pad,y+fontSize+pad*3,QString("%1x%2").arg(x/64).arg(y/64));
			p.drawLine(0,y,m_bgPixmap.width(),y);
		}
	}
	
	m_bgPixmapItem = addPixmap(m_bgPixmap);
	m_bgPixmapItem->setZValue(0);
	
	addSigMapItem();
	
	#ifdef Q_OS_ANDROID
	QString size = "64x64";
	#else
	QString size = "32x32";
	#endif
	
	// This is just a dummy pixmap - m_userItem will receive a rendered item later 
	QPixmap pix(tr(":/data/images/%1/stock-media-rec.png").arg(size));
	m_userItem = addPixmap(pix);
	m_userItem->setOffset(-(pix.width()/2.),-(pix.height()/2.));
	m_userItem->setVisible(false);
	m_userItem->setZValue(150);
	//m_userItem->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
	
	m_apLocationOverlay = addPixmap(pix);
	m_apLocationOverlay->setVisible(false);
	m_apLocationOverlay->setZValue(140);
	
	m_longPressSpinner = new LongPressSpinner();
	m_longPressSpinner->setVisible(false);
	m_longPressSpinner->setZValue(999);
	m_longPressSpinner->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
	addItem(m_longPressSpinner);
	
	m_currentMapFilename = "";
}

void MapGraphicsScene::setBgFile(QString filename)
{
	m_bgFilename = filename;
	m_bgPixmap = QPixmap(m_bgFilename);
	
	m_bgPixmapItem->setPixmap(m_bgPixmap);
	m_bgPixmapItem->setTransformationMode(Qt::SmoothTransformation);
	m_bgPixmapItem->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
	
	QSize sz = m_bgPixmap.size();
	
	setSceneRect(-sz.width() * 2., -sz.height() * 2., sz.width()*4., sz.height()*4.);
}


/// tinted() and grayscaled() from graphics-dojo on git (Qt labs) - not my creation

// Convert an image to grayscale and return it as a new image
QImage grayscaled(const QImage &image)
{
    QImage img = image;
    int pixels = img.width() * img.height();
    unsigned int *data = (unsigned int *)img.bits();
    for (int i = 0; i < pixels; ++i) {
        int val = qGray(data[i]);
        data[i] = qRgba(val, val, val, qAlpha(data[i]));
    }
    return img;
}

// Tint an image with the specified color and return it as a new image
QImage tinted(const QImage &image, const QColor &color, QPainter::CompositionMode mode = QPainter::CompositionMode_Overlay)
{
    QImage resultImage(image.size(), QImage::Format_ARGB32_Premultiplied);
    memset(resultImage.bits(), 0, resultImage.byteCount());
    QPainter painter(&resultImage);
    painter.drawImage(0, 0, grayscaled(image));
    painter.setCompositionMode(mode);
    painter.fillRect(resultImage.rect(), color);
    painter.end();
    resultImage.setAlphaChannel(image.alphaChannel());
    return resultImage;
}


void MapGraphicsScene::addApMarker(QPointF point, QString mac)
{
	MapApInfo *info = apInfo(mac);
	info->point  = point;
	info->marked = true;
	
	QImage markerGroup(":/data/images/ap-marker.png");
	markerGroup = ImageUtils::addDropShadow(markerGroup, (double)(markerGroup.width()/2));
		
	//markerGroup.save("apMarkerDebug.png");
	
	markerGroup = tinted(markerGroup, baseColorForAp(mac));

	QGraphicsPixmapItem *item = addPixmap(QPixmap::fromImage(markerGroup));
#ifdef ITEMS_SCALE_INVARIENT
	item->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
#endif
	
	double w2 = (double)(markerGroup.width())/2.;
	double h2 = (double)(markerGroup.height())/2.;
	item->setPos(point);
	item->setOffset(-w2,-h2);
	
// 	item->setFlag(QGraphicsItem::ItemIsMovable);
// 	item->setFlag(QGraphicsItem::ItemSendsGeometryChanges);
// 	item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	item->setZValue(200);
}


void MapGraphicsScene::addSigMapItem()
{
	
	m_sigMapItem = new SigMapItem();
	addItem(m_sigMapItem);
	
/*	QPixmap empty(m_bgPixmap.size());
	empty.fill(Qt::transparent);
	
	m_sigMapItem = addPixmap(empty); 
	m_sigMapItem->setTransformationMode(Qt::SmoothTransformation);
	m_sigMapItem->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
*/
	m_sigMapItem->setZValue(1);
	m_sigMapItem->setOpacity(0.75);
}

void MapGraphicsScene::scanFinished(QList<WifiDataResult> results)
{
	
	QPointF realPoint;

	if(!m_scanIf.dataTextfile().isEmpty())
	{
		if(!m_debugFirstScan)
			return;

		m_debugFirstScan = false;
	}
	
	//qDebug() << "MapGraphicsScene::scanFinished(): currentThreadId:"<<QThread::currentThreadId();
	
	// Sort the results high-low
	qSort(results.begin(), results.end(), MapGraphicsScene_sort_WifiDataResult);
	
	m_lastScanResults = results;

	foreach(WifiDataResult result, results)
	{
		// We don't want to add this AP to the internal (and file) ap info list just because
		// the wifi card scanned it - only if the user takes a reading at this location
		if(m_apInfo.contains(result.mac))
		{
			MapApInfo *info = apInfo(result.mac);
			if(info->essid.isEmpty())
				info->essid = result.essid;
		}
	}
		
	// Used by MapGraphicsView HUD
	emit scanResultsAvailable(results);
		
	updateUserLocationOverlay(
	#ifdef Q_OS_ANDROID
		-3
	#else
		+3
	#endif
	);
	//updateApLocationOverlay();
}

void MapGraphicsScene::testUserLocatorAccuracy()
{
	//return;
	
	/*
	// Print a dump of the signal readings in CSV format,
	// suitable for importing into excel/oocalc for graphic/analysis
	foreach(MapApInfo *info, m_apInfo.values())
	{
		QPointF pnt1 = info->point;
		printf("\n\n%s,%s\n",qPrintable(info->mac),qPrintable(info->essid));
		printf("dBm,Signal%%,Dist(Ft),Angle\n");
		foreach(SigMapValue *val, m_sigValues)
		{
			if(!val->hasAp(info->mac))
				continue;
			
			QPointF pnt2 = val->point;
			QLineF line(pnt1, pnt2);

			double len = line.length() / m_pixelsPerFoot;
			double angle = line.angle();
			
			printf("%d,%f,%f,%f\n", (int)val->signalForAp(info->mac, true), val->signalForAp(info->mac),len,angle);
		}
	}

	printf("\n\n");
	//return;
	*/
	
	double sum = 0.;
	double min = 999999999.;
	double max = 0.;
	int count = 0;

	QList<double> errs;

	QList<int> bins;
	double binSize = 10.;
	
	int pointsTestedCount = 0;

	QImage imageTmp;
	
	int numSigValues = m_sigValues.size();
	
	foreach(SigMapValue *val, m_sigValues)
	{
		QList<WifiDataResult> results = val->scanResults;
		 
		// Sort the results high-low
		qSort(results.begin(), results.end(), MapGraphicsScene_sort_WifiDataResult);
		
		m_lastScanResults = results;
		
		int apsUsed = 0;

		foreach(WifiDataResult r, results)
		{
			QLineF line(val->point, apInfo(r.mac)->point);
			
			if(apInfo(r.mac)->marked)
				apsUsed ++;
			
			//m_locationCheats[r.mac] = line.length();
		}
		
		if(apsUsed < 3)
		{
			qDebug() << "MapGraphicsScene::testUserLocatorAccuracy(): Skipping reading, apsUsed < 3";
			continue;
		}
		
// 		if(apsUsed > 2)
// 		{
// 			qDebug() << "MapGraphicsScene::testUserLocatorAccuracy(): Skipping reading, apsUsed > 2";
// 			continue;
// 		}
		
		pointsTestedCount ++;
		
		// Do the location calculation and store the result in m_userLocation
		if(numSigValues < 50)
		{
			// Show images for small number of readings
			bool old = m_showMyLocation;
			m_showMyLocation = true;
			
			imageTmp = updateUserLocationOverlay(val->rxGain, true, val->point, imageTmp);
			
			m_showMyLocation = old;
		}
		else
		{
			updateUserLocationOverlay(val->rxGain, false); // false = dont update image (for faster testing)
		}
		
		QLineF errorLine(val->point, m_userLocation);
		double len     = errorLine.length(),
		       lenFoot = len / m_pixelsPerFoot;
		
		if(lenFoot < 0.0001)
			lenFoot = 0;
			
		qDebug() << "MapGraphicsScene::testUserLocatorAccuracy(): Error: "<< lenFoot<<"ft";
		
		QApplication::processEvents();
		
		if(!isnan(lenFoot))
		{
			int bin = (int)(lenFoot / binSize);
			while(bins.size() <= bin)
				bins << 0;
			bins[bin] ++;
			
			sum   += lenFoot;
			count ++;
			
			errs << lenFoot;
			
			if(lenFoot < min)
				min = lenFoot;
			if(lenFoot > max)
				max = lenFoot;
		}
		
		// Halt in no solution and display an image
// 		if((isnan(lenFoot) || lenFoot > 0))
// 		{
// 			m_showMyLocation = true;
// 			updateUserLocationOverlay(val->rxGain, true, val->point); // render images
// 			m_showMyLocation = false;
// 			return; // halt, don't continue
// 		}
		
		
	}
	
	
	
	double avg = sum / count;
	
	// find the standard deviation
	float numerator = 0;
	float denominator = (float)count;
	foreach(double len, errs)
		numerator += pow((len - avg), 2);

	// Print bins out in CSV format for copying into excel/oocalc to graph/analyize/etc
	/*
	printf("\n\nBin(Ft),Count\n");
	for(int bin=0; bin<bins.size(); bin++)
		printf("%d,%d\n", (int)(bin * binSize), bins[bin]);
	printf("\n\n");
	*/

	// TODO Generate our own simple graph in a QImage and display it in a dialog below
	
	float stdDev = sqrt(numerator/denominator);
	
	float bottomAvg95 = avg - (stdDev*2),
	         topAvg95 = avg + (stdDev*2);

	if(bottomAvg95 < 0)
		bottomAvg95 = 0;

	//int maxCount = m_sigValues.size();
	int maxCount = pointsTestedCount;
	double solveRatio = (double)count / (double)maxCount;
	double solvePerc = (int)(solveRatio * 100);
	
	QString results = QString("Accuracy (Error): \n   Min: %2 ft\n   Max: %3 ft\n   Std dev: %4 ft\n    Avg: %1 ft\n   95% Radius: %5 ft - %6 ft\n    # Solutions: %7/%8 %9%\n\nSummary: #%9%: %1/%4 (%5-%6)")
		.arg((int)avg)
		.arg((int)min)
		.arg((int)max)
		.arg(stdDev)
		.arg((int)bottomAvg95)
		.arg((int)topAvg95)
		.arg(count)
		.arg(maxCount)
		.arg(solvePerc);
		
	qDebug() << "MapGraphicsScene::testUserLocatorAccuracy(): Error stats: min:"<<min<<"ft, max:"<<max<<"ft, std dev:"<<stdDev<<"ft, avg:"<<avg<<"ft, 95% radius:"<<bottomAvg95<<"-"<<topAvg95<<"ft, # solutions:"<<count<<"/"<<maxCount<<", "<<solvePerc<<"%";
	qDebug() << "\tSummary: "<<solvePerc<<"%: "<<avg<<"/"<<stdDev<<" ("<<bottomAvg95<<"/"<<topAvg95<<")\n";
	
	QApplication::processEvents();
	
	QMessageBox::information(0, "Accuracy Results", results);

	m_locationCheats.clear();
}
	
	
void MapGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent * mouseEvent)
{
	if(m_longPressTimer.isActive())
	{
		m_longPressTimer.stop();
		m_longPressCountTimer.stop();
	}
	m_longPressTimer.start();
	m_pressPnt = mouseEvent->lastScenePos();
	
	m_longPressSpinner->setPos(m_pressPnt);
	
	m_longPressCountTimer.start();
	m_longPressCount = 0;
	
	QGraphicsScene::mousePressEvent(mouseEvent);
}

void MapGraphicsScene::invalidateLongPress(QPointF curPoint)
{
	if(m_longPressTimer.isActive())
	{
		if(!curPoint.isNull())
		{
			#ifdef Q_OS_ANDROID
			const double maxDist = 16.;
			#else
			const double maxDist = 1.;
			#endif
			
			// Don't invalidate longpress if curPoint (current mouse/finger position)
			// is within 'maxDist' radius of the starting location
			if(QLineF(m_pressPnt, curPoint).length() < maxDist)
				return;
		}
		
		m_longPressTimer.stop();
		m_longPressCountTimer.stop();
		m_longPressSpinner->setVisible(false);
		
		m_mapWindow->clearStatusMessage();
	}
}

void MapGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent * mouseEvent)
{
	invalidateLongPress();
		
	QGraphicsScene::mouseReleaseEvent(mouseEvent);
}

void MapGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent * mouseEvent)
{
	invalidateLongPress(mouseEvent->lastScenePos());
		
	QGraphicsScene::mouseMoveEvent(mouseEvent);
}

void MapGraphicsScene::longPressCount()
{
	m_longPressCount ++;
	int maxCount = m_longPressTimer.interval() / m_longPressCountTimer.interval();
	double part  = ((double)m_longPressCount) / ((double)maxCount);
	
	m_longPressSpinner->setProgress(part);
	
	int percent = (int)(part * 100.); 
	
	if(percent > 100.) // we missed a cancel call somehwere...
	{
		m_longPressCountTimer.stop();
	}
	else
	{
		m_mapWindow->setStatusMessage(tr("Waiting %1%").arg(percent), m_longPressTimer.interval());
	}
}

void MapGraphicsScene::longPressTimeout()
{
	m_longPressSpinner->setGoodPress(true);

	if(m_markApMode)
	{
		/// Scan for APs nearby and prompt user to choose AP
		
		m_mapWindow->setStatusMessage(tr("<font color='green'>Scanning...</font>"));
		
		QList<WifiDataResult> results = m_scanIf.scanResults();
		m_mapWindow->setStatusMessage(tr("<font color='green'>Scan finished!</font>"), 3000);
		
		if(results.isEmpty())
		{
			m_mapWindow->flagApModeCleared();
			m_mapWindow->setStatusMessage(tr("<font color='red'>No APs found nearby</font>"), 3000);
		}
		else
		{
			QStringList items;
			foreach(WifiDataResult result, results)
				items << QString("%1%: %2 - %3").arg(QString().sprintf("%02d", (int)(result.value*100))).arg(result.mac/*.left(6)*/).arg(result.essid);
			
			qSort(items.begin(), items.end());
			
			// Reverse order so strongest appears on top
			QStringList tmp;
			for(int i=items.size()-1; i>-1; i--)
				tmp.append(items[i]);
			items = tmp;

			
			bool ok;
			QString item = QInputDialog::getItem(0, tr("Found APs"),
								tr("AP:"), items, 0, false, &ok);
			if (ok && !item.isEmpty())
			{
				QStringList pair = item.split(" ");
				if(pair.size() < 2) // problem getting MAC
					ok = false;
				else
				{
					// If user chooses AP, call addApMarker(QString mac, QPoint location);
					QString mac = pair[1];
					
					// Find result just to tell user ESSID
					WifiDataResult matchingResult;
					foreach(WifiDataResult result, results)
						if(result.mac == mac)
							matchingResult = result;
					
					// Add to map and ap list
					addApMarker(m_pressPnt, mac);
					
					// Update UI
					m_mapWindow->flagApModeCleared();
					m_mapWindow->setStatusMessage(tr("Added %1 (%2)").arg(mac).arg(matchingResult.valid ? matchingResult.essid : "Unknown ESSID"), 3000);
					
					// Render map overlay (because the AP may be tied to an existing scan result)
					//#ifndef Q_OS_ANDROID
					triggerRender();
					//#endif
					//renderSigMap();
				}
			}
			else
				ok = false;
			
			if(!ok)
			{
				m_mapWindow->flagApModeCleared();
				m_mapWindow->setStatusMessage(tr("User canceled"), 3000);
			}
		}
	}
	else
	/// Not in "mark AP" mode - regular scan mode, so scan and add store results
	{
		// scan for APs nearby
		m_mapWindow->setStatusMessage(tr("<font color='green'>Scanning...</font>"));
		
		QList<WifiDataResult> results = m_scanIf.scanResults();
		m_mapWindow->setStatusMessage(tr("<font color='green'>Scan finished!</font>"), 3000);
		
		if(results.size() > 0)
		{
			// call addSignalMarker() with presspnt
			addSignalMarker(m_pressPnt, results);
			
			m_mapWindow->setStatusMessage(tr("Added marker for %1 APs").arg(results.size()), 3000);
			
			// Render map overlay
			//#ifndef Q_OS_ANDROID
			triggerRender();
			//#endif
			//renderSigMap();
			
			QStringList notFound;
			foreach(WifiDataResult result, results)
				if(!apInfo(result)->marked)
					notFound << QString("<font color='%1'>%2</font> (<i>%3</i>)")
						.arg(colorForSignal(result.value, result.mac).name())
						.arg(result.mac.right(6))
						.arg(result.essid); // last two octets of mac should be enough
					
			if(notFound.size() > 0)
			{
				m_mapWindow->setStatusMessage(tr("Ok, but %2 APs need marked: %1").arg(notFound.join(", ")).arg(notFound.size()), 10000);
			}
		}
		else
		{
			m_mapWindow->setStatusMessage(tr("<font color='red'>No APs found nearby</font>"), 3000);
		}
	}
}

void MapGraphicsScene::setRenderMode(RenderMode r)
{
	if(r != m_renderMode)
	{
		m_colorListForMac.clear(); // color gradient style change based on render mode
		m_renderMode = r;
		triggerRender();
		
		QSettings("wifisigmap").setValue("rendermode", (int)r);
	}
}

QImage MapGraphicsScene::renderSignalMarker(QList<WifiDataResult> results)
{
#ifdef Q_OS_ANDROID
	const int iconSize = 64;
#else
	const int iconSize = 32;
#endif

	const double iconSizeHalf = ((double)iconSize)/2.;
		
	int numResults = results.size();
	
	int resultCounter = 0;
	
	for(int resultIdx=0; resultIdx<numResults; resultIdx++)
		if(apInfo(results[resultIdx].mac)->renderOnMap)
			resultCounter ++;
				
	double angleStepSize = 360. / ((double)resultCounter);
	
	resultCounter = 0;
		
	QRectF boundingRect;
	QList<QRectF> iconRects;
	for(int resultIdx=0; resultIdx<numResults; resultIdx++)
	{
		if(apInfo(results[resultIdx].mac)->renderOnMap)
		{
			double rads = ((double)resultCounter) * angleStepSize * 0.0174532925;
			double iconX = iconSizeHalf/2.5 * numResults * cos(rads);
			double iconY = iconSizeHalf/2.5 * numResults * sin(rads);
			QRectF iconRect = QRectF(iconX - iconSizeHalf, iconY - iconSizeHalf, (double)iconSize, (double)iconSize);
			iconRects << iconRect;
			boundingRect |= iconRect;
			
			resultCounter ++;
		}
	}
	
 	if(resultCounter == 1)
 		boundingRect = iconRects[0] = QRectF(-iconSizeHalf, -iconSizeHalf, (double)iconSize, (double)iconSize);
	
	boundingRect.adjust(-10,-10,+10,+10);
	
//	qDebug() << "MapGraphicsScene::addSignalMarker(): boundingRect:"<<boundingRect;
	
	QImage markerGroup(boundingRect.size().toSize(), QImage::Format_ARGB32_Premultiplied);
	memset(markerGroup.bits(), 0, markerGroup.byteCount());//markerGroup.fill(Qt::green);
	QPainter p(&markerGroup);
	
#ifdef Q_OS_ANDROID
	QFont font("", 6, QFont::Bold);
	QFont smallFont("", 4, QFont::Bold);
#else
	QFont font("", (int)(iconSize*.33), QFont::Bold);
	QFont smallFont("", (int)(iconSize*.20));
#endif

	p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);

	
	double zeroAdjX = boundingRect.x() < 0. ? fabs(boundingRect.x()) : 0.0; 
	double zeroAdjY = boundingRect.y() < 0. ? fabs(boundingRect.y()) : 0.0;
	
	resultCounter = 0;
	for(int resultIdx=0; resultIdx<numResults; resultIdx++)
	{
		WifiDataResult result = results[resultIdx];
		if(!apInfo(results[resultIdx].mac)->renderOnMap)
			continue;
		
		QColor centerColor = colorForSignal(result.value, result.mac);
		
		QRectF iconRect = iconRects[resultCounter];
		iconRect.translate(zeroAdjX,zeroAdjY);
		
		//qDebug() << "MapGraphicsScene::addSignalMarker(): resultIdx:"<<resultIdx<<": iconRect:"<<iconRect<<", centerColor:"<<centerColor;
		
		p.save();
		{
			p.translate(iconRect.topLeft());
			
			// Draw inner gradient
			QRadialGradient rg(QPointF(iconSize/2,iconSize/2),iconSize);
			rg.setColorAt(0, centerColor/*.lighter(100)*/);
			rg.setColorAt(1, centerColor.darker(500));
			//p.setPen(Qt::black);
			p.setBrush(QBrush(rg));
			
			// Draw outline
			p.setPen(QPen(Qt::black,1.5));
			p.drawEllipse(0,0,iconSize,iconSize);
			
			// Render signal percentage
			//QString sigString = QString("%1%").arg((int)(result.value * 100));
			QString sigString = QString("%1d").arg((int)(result.dbm));
			
			// Calculate text location centered in icon
			p.setFont(font);
	
			QRect textRect = p.boundingRect(0, 0, INT_MAX, INT_MAX, Qt::AlignLeft, sigString);
			int textX = (int)(iconRect.width()/2  - textRect.width()/2  + iconSize*.1); // .1 is just a cosmetic adjustment to center it better
			#ifdef Q_OS_ANDROID
			int textY = (int)(iconRect.height()/2 - textRect.height()/2 + font.pointSizeF() + 16);
			#else
			int textY = (int)(iconRect.height()/2 - textRect.height()/2 + font.pointSizeF() * 1.33);
			#endif
			
			qDrawTextO(p,textX,textY,sigString);
			
			// Render AP ESSID
			QString essid = result.essid;
			
			// Calculate text location centered in icon
			p.setFont(smallFont);
			
			textRect = p.boundingRect(0, 0, INT_MAX, INT_MAX, Qt::AlignLeft, essid);
			textX = (int)(iconRect.width()/2  - textRect.width()/2  + font.pointSizeF()*.1); // .1 is just a cosmetic adjustment to center it better
			#ifdef Q_OS_ANDROID
			textY += font.pointSizeF() * 2;
			#else
			textY += (int)(font.pointSizeF()*0.95);
			#endif
			
			qDrawTextO(p,textX,textY,essid);
			
			
			//p.restore();
			/*
			p.setPen(Qt::green);
			p.setBrush(QBrush());
			p.drawRect(marker.rect());
			*/
		}
		p.restore();
		
		resultCounter ++;
	}
	
	p.end();
	
	markerGroup = ImageUtils::addDropShadow(markerGroup, (double)iconSize / 2.);
	
	return markerGroup;
}

SigMapValue *MapGraphicsScene::addSignalMarker(QPointF point, QList<WifiDataResult> results)
{	
	
	QImage markerGroup = renderSignalMarker(results);
	
	//markerGroup.save("markerGroupDebug.jpg");
	
	// Create value record and graphics item
	SigMapValue *val = new SigMapValue(point, results);
	SigMapValueMarker *item = new SigMapValueMarker(val, QPixmap::fromImage(markerGroup));
	
	// Add to graphics scene
	addItem(item);
	
	// Place item->pos() at the center instead of top-left
	double w2 = (double)(markerGroup.width())/2.;
	double h2 = (double)(markerGroup.height())/2.;
	item->setPos(point);
	item->setOffset(-w2,-h2);
	
	// Possible future use:
	//item->setFlag(QGraphicsItem::ItemIsMovable);
	
	// Optimize modes and tweak flags 
	item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
 	item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
#ifdef ITEMS_SCALE_INVARIENT
	item->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
#endif
	item->setZValue(199);
	
	//item->setOpacity(0);
	if(!m_renderOpts.showReadingMarkers)
		item->setVisible(false);
	
	// Add pointer to the item in the scene to the signal value for turning on/off per user
	val->marker = item;
	
	// Add to list of readings
	m_sigValues << val;
	
	if(m_apGuessUpdateTimer.isActive())
		m_apGuessUpdateTimer.stop();
	m_apGuessUpdateTimer.start();
	
	return val;
}


/// SigMapValue method implementations:

bool SigMapValue::hasAp(QString mac)
{
	foreach(WifiDataResult result, scanResults)
		if(result.mac == mac)
			return true;
	
	return false;
}

double SigMapValue::signalForAp(QString mac, bool returnDbmValue)
{
	foreach(WifiDataResult result, scanResults)
		if(result.mac == mac)
			return returnDbmValue ?
				result.dbm  :
				result.value;
	
	return 0.0;
}

/// End SigMapValue impl



SigMapValue *MapGraphicsScene::findNearest(QPointF match, QString apMac)
{
	if(match.isNull())
		return 0;
		
	SigMapValue *nearest = 0;
	double minDist = (double)INT_MAX;
	
	foreach(SigMapValue *val, m_sigValues)
	{
		if(val->consumed || !val->hasAp(apMac))
			continue;
			
		double dx = val->point.x() - match.x();
		double dy = val->point.y() - match.y(); 
		double dist = dx*dx + dy*dy; // dont need true dist, so dont sqrt
		if(dist < minDist)
		{
			nearest = val;
			minDist = dist;
		}
	}
	
	if(nearest)
		nearest->consumed = true;
	
	return nearest;
	
}



void MapGraphicsScene::renderProgress(double value)
{
	(void)value;
	//m_mapWindow->setStatusMessage(tr("Rendering %1%").arg((int)(value*100)), 250);
}

// void MapGraphicsScene::renderComplete(QImage mapImg)
// {
// 	qDebug() << "MapGraphicsScene::renderComplete(): currentThreadId:" << QThread::currentThreadId();
// 	m_sigMapItem->setPixmap(QPixmap::fromImage(mapImg));
// 	//m_sigMapItem->setPicture(mapImg);
// 	m_mapWindow->setStatusMessage(tr("Signal map updated"), 500);
// }

void MapGraphicsScene::renderComplete(QPicture pic)
{
	qDebug() << "MapGraphicsScene::renderComplete(): currentThreadId:" << QThread::currentThreadId();
	//m_sigMapItem->setPixmap(QPixmap::fromImage(mapImg));
	m_sigMapItem->setPicture(pic);
	m_mapWindow->setStatusMessage(tr("Signal map updated"), 500);
	
	if(m_firstRender) // first render after load, init view
	{
		m_mapWindow->gv()->reset();
		m_mapWindow->gv()->scaleView(m_viewScale);
		m_mapWindow->gv()->horizontalScrollBar()->setValue(m_scrollHPos);
		m_mapWindow->gv()->verticalScrollBar()->setValue(m_scrollVPos);
		m_firstRender = false;
	}
}


/// SigMapItem impl

SigMapItem::SigMapItem()
{
	m_internalCache = true;
}

void SigMapItem::setInternalCache(bool flag)
{
	m_internalCache = flag;
	setPicture(m_pic);
}

void SigMapItem::setPicture(QPicture pic)
{
	setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	//setCacheMode(QGraphicsItem::ItemCoordinateCache);
	prepareGeometryChange();
	m_pic = pic;
	
	m_offset = QPoint();
	
	if(m_internalCache)
	{
		QRect picRect = m_pic.boundingRect();
		if(picRect.isValid())
		{
			if(picRect.size().width() * picRect.size().height() * 3 > 128*1024*1024)
			{
				qDebug() << "SigMapItem::setPicture(): Size too large, not setting: "<<picRect.size();
				m_img = QImage();
				update();
				return;
			}
			
			m_offset = picRect.topLeft();
			
			QImage img(picRect.size(), QImage::Format_ARGB32_Premultiplied);
			QPainter p(&img);
			p.fillRect(img.rect(), Qt::transparent);
			p.drawPicture(-m_offset, m_pic);
			p.end();
			m_img = img;
		}
		
		//qDebug() << "SigMapItem::setPicture(): m_img.size():"<<m_img.size()<<", picRect:"<<picRect;
		//m_img.save("mapImg.jpg");
	}
	else
		m_img = QImage();
	
	update();
}
	
// void SigMapItem::setPicture(QImage img)
// {
// 	m_img = img;
// 	update();
// }


QRectF SigMapItem::boundingRect() const
{
	return m_pic.boundingRect();
	//return m_img.rect();	
}
	
void SigMapItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget */*widget*/)
{
	//qDebug() << "SigMapItem::paint: option->exposedRect:"<<option->exposedRect<<", m_offset:"<<m_offset<<", m_img.size():"<<m_img.size();
		
	painter->save();
	//painter->setOpacity(0.75);
	painter->setClipRect( option->exposedRect );
	
	if(m_internalCache && 
	  !m_img.isNull())
	{
		/*
		if(option->exposedRect.isValid())
			painter->drawImage( option->exposedRect.translated(m_offset), m_img, option->exposedRect.toRect() );
		else
		*/
			painter->drawImage( m_offset, m_img );
	}
	else
	if(!m_pic.isNull())
		painter->drawPicture( m_offset, m_pic );

// 	if(!m_img.isNull())
// 	{
// // 		if(option->exposedRect.isValid())
// // 			painter->drawImage( option->exposedRect, m_img, option->exposedRect.toRect() );
// // 		else
// 			painter->drawImage( 0, 0, m_img );
// 	}

	painter->restore();
}

/// End SigMapItem impl

bool MapGraphicsScene::pauseRenderUpdates(bool flag)
{
	bool oldFlag = m_renderUpdatesPaused;
	m_renderUpdatesPaused = flag;
	
	if(!flag)
	{
		triggerRender();
		m_updateSignalMarkerTimer.start();
	}
	
	return oldFlag;
}

void MapGraphicsScene::setRenderAp(MapApInfo *info, bool flag)
{
	info->renderOnMap = flag;
	
	if(!m_renderUpdatesPaused)
	{
		if(m_updateSignalMarkerTimer.isActive())
			m_updateSignalMarkerTimer.stop();
		m_updateSignalMarkerTimer.start();
		
		triggerRender();
	}
}

void MapGraphicsScene::updateSignalMarkers()
{
	foreach(SigMapValue *val, m_sigValues)
	{
		// Re-render marker because the 'renderOnMap' flag for the AP contained therein may have changed
		QImage markerGroup = renderSignalMarker(val->scanResults);
		val->marker->setPixmap(QPixmap::fromImage(markerGroup));
		
		double w2 = (double)(markerGroup.width())/2.;
		double h2 = (double)(markerGroup.height())/2.;
		val->marker->setOffset(-w2,-h2);
	}
}

void MapGraphicsScene::setRenderAp(QString mac, bool flag)
{
	//apInfo(mac)->renderOnMap = flag;
	setRenderAp(apInfo(mac), flag);
}

MapApInfo* MapGraphicsScene::apInfo(WifiDataResult r)
{
	if(m_apInfo.contains(r.mac))
		return m_apInfo.value(r.mac);
		
	MapApInfo *inf = new MapApInfo(r);
	m_apInfo[r.mac] = inf;
	 
	return inf;
}

MapApInfo* MapGraphicsScene::apInfo(QString apMac)
{
	if(m_apInfo.contains(apMac))
		return m_apInfo.value(apMac);
		
	MapApInfo *inf = new MapApInfo();
	inf->mac = apMac;
	m_apInfo[apMac] = inf;
	 
	return inf;
}

QColor MapGraphicsScene::baseColorForAp(QString apMac)
{
	QColor baseColor;
	
	MapApInfo *info = apInfo(apMac);
	
	if(info->color.isValid())
	{
		baseColor = info->color;
		//qDebug() << "MapGraphicsScene::colorForSignal: "<<apMac<<": Loaded base "<<baseColor<<" from m_apMasterColor";
	} 
	else
	{
		bool foundColor = false;
		while(m_colorCounter < m_masterColorsAvailable.size()-1)
		{
			baseColor = m_masterColorsAvailable[m_colorCounter ++];
			
			bool colorInUse = false;
			foreach(MapApInfo *info, m_apInfo)
				if(info->color == baseColor)
					colorInUse = true;
					
			if(!colorInUse)
			{
				foundColor = true;
				break;
			}
		}
		
		if(foundColor)
		{
			qDebug() << "MapGraphicsScene::baseColorForAp: "<<apMac<<": Using NEW base "<<baseColor.name()<<" from m_masterColorsAvailable # "<<m_colorCounter;
		}
		//if(!foundColor)
		else
		{
			baseColor = QColor::fromHsv(qrand() % 359, 255, 255);
			qDebug() << "MapGraphicsScene::baseColorForAp: "<<apMac<<": Using NEW base "<<baseColor.name()<<" (random HSV color)";
		}
		
		//m_apMasterColor[apMac] = baseColor;
		info->color = baseColor;
		//qDebug() << "MapGraphicsScene::colorForSignal: "<<apMac<<": Stored baseColor "<<m_apMasterColor[apMac]<<" in m_apMasterColor["<<apMac<<"]";
	}
	
	return baseColor;
}

QColor MapGraphicsScene::colorForSignal(double sig, QString apMac)
{
	QList<QColor> colorList;
	if(!m_colorListForMac.contains(apMac))
	{
		//qDebug() << "MapGraphicsScene::colorForSignal: "<<apMac<<": m_colorListforMac cache miss, getting base...";
		QColor baseColor = apMac.isEmpty() ? Qt::transparent : baseColorForAp(apMac);
		
		// paint the gradient
		#ifdef Q_OS_ANDROID
		int imgHeight = 1;
		#else
		int imgHeight = 10; // for debugging output
		#endif
		
		QImage signalLevelImage(100,imgHeight,QImage::Format_ARGB32_Premultiplied);
		QPainter sigPainter(&signalLevelImage);
		
		if(1/*m_renderMode == RenderCircles ||
		   m_renderMode == RenderTriangles*/)
		{
			QLinearGradient fade(QPoint(0,0),QPoint(100,0));
	// 		fade.setColorAt( 0.3, Qt::black  );
	// 		fade.setColorAt( 1.0, Qt::white  );
			fade.setColorAt( 0.0, Qt::black );
			fade.setColorAt( 0.1, Qt::red    );
			fade.setColorAt( 0.15, QColor(255,2,195)); //dark pink
			fade.setColorAt( 0.2, QColor(191,0,254)); //purple
			fade.setColorAt( 0.25, QColor(98,2,254)); //deep blue/purple
			fade.setColorAt( 0.3, QColor(0,12,254)); //bright blue
			fade.setColorAt( 0.35, QColor(0,237,254)); //cyan
			fade.setColorAt( 0.6, Qt::yellow);
			fade.setColorAt( 0.8, QColor(0,254,89)); //light green
			fade.setColorAt( 0.9, QColor(0,254,0)); //solid green
			fade.setColorAt( 1.0, QColor(12,255,0)); //bright green
			sigPainter.fillRect( signalLevelImage.rect(), fade );
			
			if(baseColor != Qt::transparent)
			{
				sigPainter.setCompositionMode(QPainter::CompositionMode_HardLight);
				sigPainter.fillRect( signalLevelImage.rect(), baseColor );
			}
		
		}
		else
		{
			//sigPainter.fillRect( signalLevelImage.rect(), baseColor ); 
		
			QLinearGradient fade(QPoint(0,0),QPoint(100,0));
			fade.setColorAt( 1.0, baseColor.lighter(100)  );
	 		fade.setColorAt( 0.5, baseColor               );
	 		//fade.setColorAt( 0.0, Qt::transparent  );
	 		fade.setColorAt( 0.0, baseColor.darker(300)   );
			sigPainter.fillRect( signalLevelImage.rect(), fade );
			//sigPainter.setCompositionMode(QPainter::CompositionMode_HardLight);
			//sigPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		}
		
		sigPainter.end();
		
		#ifndef Q_OS_ANDROID
		// just for debugging (that's why the img is 10px high - so it is easier to see in this output)
		signalLevelImage.save(tr("signalLevelImage-%1.png").arg(apMac));
		#endif
		
		QRgb* scanline = (QRgb*)signalLevelImage.scanLine(0);
		for(int i=0; i<100; i++)
		{
			QColor color;
			color.setRgba(scanline[i]);
			colorList << color;
		}
		
		m_colorListForMac.insert(apMac, colorList);
	}
	else
	{
		colorList = m_colorListForMac.value(apMac); 
	}
	
	int colorIdx = (int)(sig * 100);
	if(colorIdx > 99)
		colorIdx = 99;
	if(colorIdx < 0)
		colorIdx = 0;
	
	return colorList[colorIdx];
}


QString qPointFToString(QPointF point)
{
	return QString("%1,%2").arg(point.x()).arg(point.y());
}

QPointF qPointFFromString(QString string)
{
	QStringList list = string.split(",");
	if(list.length() < 2)
		return QPointF();
	return QPointF(list[0].toDouble(),list[1].toDouble());
}

void MapGraphicsScene::saveResults(QString filename)
{
	m_currentMapFilename = filename;
	QFileInfo info(filename);
	if(info.exists() && !info.isWritable())
	{
		QMessageBox::critical(0, "Unable to Save", tr("Unable to save %1 - it's not writable!").arg(filename));
		return;
	}
	
	QSettings data(filename, QSettings::IniFormat);
	
	qDebug() << "MapGraphicsScene::saveResults(): Saving to: "<<filename;
	
	// Store bg filename
	data.setValue("background", m_bgFilename);
	
	// Store window scroll/zoom location
	data.setValue("h-pos", m_mapWindow->gv()->horizontalScrollBar()->value());
	data.setValue("v-pos", m_mapWindow->gv()->verticalScrollBar()->value());
	data.setValue("scale", m_mapWindow->gv()->scaleFactor());
	
	// Store view cale translation values
	/// TODO make user configurable
	data.setValue("footpx",  m_pixelsPerFoot);
	data.setValue("meterpx", m_pixelsPerMeter);
	
	int idx = 0;
	
	// Save AP locations
	data.beginWriteArray("ap-locations");
	foreach(MapApInfo *info, m_apInfo)
	{
		data.setArrayIndex(idx++);
		data.setValue("mac",		info->mac);
		data.setValue("essid",		info->essid);
		data.setValue("marked",		info->marked);
		data.setValue("point",		qPointFToString(info->point));
		data.setValue("color", 		info->color.name());
		data.setValue("renderap",	info->renderOnMap);
		data.setValue("mfg",		info->mfg);
		data.setValue("txpower",	info->txPower);
		data.setValue("txgain",		info->txGain);
		data.setValue("lossfactor",	qPointFToString(info->lossFactor));
		data.setValue("shortcutoff",	info->shortCutoff);
		data.setValue("locationguess",	qPointFToString(info->locationGuess));
	}
	data.endArray();
		
	// Save signal readings
	idx = 0;
	data.beginWriteArray("readings");
	foreach(SigMapValue *val, m_sigValues)
	{
		data.setArrayIndex(idx++);
		data.setValue("point",		qPointFToString(val->point));
		data.setValue("rxgain",		val->rxGain);
		data.setValue("rxmac",		val->rxMac);
		data.setValue("timestamp",	val->timestamp.toTime_t());
		data.setValue("lat",		val->lat);
		data.setValue("lng",		val->lng);
		
		int resultIdx = 0;
		data.beginWriteArray("signals");
		foreach(WifiDataResult result, val->scanResults)
		{
			data.setArrayIndex(resultIdx++);
			foreach(QString key, result.rawData.keys())
			{
				data.setValue(key, result.rawData.value(key));
			}
		}
		data.endArray();
	}
	data.endArray();
}

void MapGraphicsScene::loadResults(QString filename)
{
	//m_mapWindow->gv()->resetMatrix();
	m_mapWindow->gv()->resetTransform();
	qDeleteAll(m_apInfo.values());
	m_apInfo.clear();
	
	m_colorCounter = 0; // reset AP color counter
	
	QSettings data(filename, QSettings::IniFormat);
	qDebug() << "MapGraphicsScene::loadResults(): Loading from: "<<filename;
	
	QFileInfo info(filename);
	if(info.exists() && !info.isWritable())
	{
		QMessageBox::warning(0, "Warning", tr("%1 is READ ONLY - you won't be able to save any changes!").arg(filename));
	}
	
	
	clear(); // clear and reset the map
	
	// Set current filename after clear() because clear() clears m_currentMapFilename
	m_currentMapFilename = filename;

	// Load background file
	QString bg = data.value("background").toString();
	if(!bg.isEmpty())
		setBgFile(bg);
		
	// Load scroll/zoom locations
	double scale = data.value("scale", 1.0).toDouble();
	m_mapWindow->gv()->scaleView(scale);
	
	int hPos = data.value("h-pos", 0).toInt();
	int vPos = data.value("v-pos", 0).toInt();

	// Store for setting up the view after render because
	// the real "pixel meaning" of hPos may change if the render resizes the scene rect,
	// so reapply after the scene rect may have changed for the first time.
	m_firstRender = true;
	m_scrollHPos = hPos;
	m_scrollVPos = vPos;
	m_viewScale = scale;

	m_mapWindow->gv()->reset();
	m_mapWindow->gv()->scaleView(m_viewScale);
	m_mapWindow->gv()->horizontalScrollBar()->setValue(m_scrollHPos);
	m_mapWindow->gv()->verticalScrollBar()->setValue(m_scrollVPos);
	
	// Read scale translation values
	m_pixelsPerFoot  = data.value("footpx",  13.).toDouble();
	m_pixelsPerMeter = data.value("meterpx", 42.).toDouble();
	
	// Load ap locations
	int numApLocations = data.beginReadArray("ap-locations");
	
	qDebug() << "MapGraphicsScene::loadResults(): Reading numApLocations: "<<numApLocations;
	for(int i=0; i<numApLocations; i++)
	{
		data.setArrayIndex(i);
		
		QString pointString = data.value("point").toString();
		if(pointString.isEmpty())
			// prior to r38, point key was "center"
			pointString = data.value("center").toString();
		
		QPointF point  = qPointFFromString(pointString);
		QString apMac  = data.value("mac").toString();
		QString essid  = data.value("essid").toString();
		bool marked    = data.value("marked", !point.isNull()).toBool();
		QColor color   = data.value("color").toString();
		bool render    = data.value("renderap", true).toBool();
		
		QString mfg    = data.value("mfg", "").toString(); // TODO build chart of [txpower,txgain] based on mfg [which can be drived from MAC OUI]
		double txPower = data.value("txpower",	11.8).toDouble();  // dBm, 11.9 dBm = 19 mW (approx) = Linksys WRT54 Default TX power (approx)
		double txGain  = data.value("txgain",    5.0).toDouble();  // dBi,  5.0 dBi = Linksys WRT54 Stock Antenna Gain (approx)
		QPointF lossf  = qPointFFromString(data.value("lossfactor", "").toString());
				// lossfactor.x() and .y() are arbitrary tuning parameters X and Y, typically in the range of [-1.0,5.0]
				// (formula variable 'n' for short and far dBms, respectively)
		int shortCutoff = data.value("shortcutoff", -49).toInt();
				   // dBm (typically -49) value at which to swith from loss factor X to loss factor Y for calculating distance 
				   // (less than shortCutoff [close to AP], use X, greater [farther from AP] - use Y)
		QPointF guess  = qPointFFromString(data.value("locationguess", "").toString());

		if(apMac.isEmpty())
			continue;
		
		if(!color.isValid())
		{
			qDebug() << "MapGraphicsScene::loadResults(): Getting base color for AP#:"<<i<<", mac:"<<apMac;
			color = baseColorForAp(apMac);
		}
		
		MapApInfo *info = apInfo(apMac);
		
		info->essid	= essid;
		info->marked	= marked;
		info->point	= point;
		info->color	= color;
		info->renderOnMap = render;
		info->mfg	= mfg;
		info->txPower	= txPower;
		info->txGain	= txGain;
		info->lossFactor = lossf;
		info->shortCutoff = shortCutoff;
		info->locationGuess = guess;
		
		if(marked)
			addApMarker(point, apMac);
	}
	data.endArray();
	
	// Load signal readings
	int numReadings = data.beginReadArray("readings");
	qDebug() << "MapGraphicsScene::loadResults(): Reading numReadings: "<<numReadings;
	for(int i=0; i<numReadings; i++)
	{
		/*
		qDebug() << "MapGraphicsScene::loadResults(): i: "<<i<<" / "<<numReadings;
		if(i != numReadings - 3)
			continue; /// NOTE just for debugging triangulation/trilateration - REMOVE for production!
		*/	
		data.setArrayIndex(i);
		//qDebug() << "MapGraphicsScene::loadResults(): Reading point#: "<<i;
		
		// Load location of this reading
		QPointF point = qPointFFromString(data.value("point").toString());
		
		// Load other fields
		double rxGain  = data.value("rxgain",-3.).toDouble();
		QString rxMac  = data.value("rxMac","").toString();
		uint timestamp = data.value("timestamp",0).toUInt();
		double lat     = data.value("lat",0.).toDouble();
		double lng     = data.value("lng",0.).toDouble();
		
		int numSignals = data.beginReadArray("signals");
		
		QList<WifiDataResult> results;
		for(int j=0; j<numSignals; j++)
		{
			data.setArrayIndex(j);
			QStringList keys = data.childKeys();
			QStringHash rawData;
			
			foreach(QString key, keys)
			{
				rawData[key] = data.value(key).toString();
			}
			
			WifiDataResult result;
			result.loadRawData(rawData);
			results << result;
			
			// Patch old data files
			MapApInfo *info = apInfo(result.mac);
			if(info->essid.isEmpty())
				info->essid = result.essid;
		}
		data.endArray();
		
		SigMapValue *val = addSignalMarker(point, results);
		
		val->rxGain	= rxGain;
		val->rxMac	= rxMac;
		val->timestamp	= QDateTime::fromTime_t(timestamp);
		val->lat	= lat;
		val->lng	= lng;
		
	}
	data.endArray();
	
	// Call for render of map overlay
	triggerRender();
}


void MapRenderOptions::loadFromQSettings()
{
	QSettings settings("wifisigmap");
	
	cacheMapRender     	= settings.value("ropts-cacheMapRender", 	true).toBool();
#ifdef Q_OS_ANDROID
	// default to false for some debugging I'm working on
	showReadingMarkers 	= false; //settings.value("ropts-showReadingMarkers", 	true).toBool();
#else
	showReadingMarkers 	= settings.value("ropts-showReadingMarkers", 	true).toBool();
#endif

	multipleCircles		= settings.value("ropts-multipleCircles", 	false).toBool();
	fillCircles		= settings.value("ropts-fillCircles", 		true).toBool();
	radialCircleSteps	= settings.value("ropts-radialCircleSteps", 	4 * 4 * 4).toInt();
	radialLevelSteps	= settings.value("ropts-radialLevelSteps", 	(int)(100 / .25)).toInt();
	radialAngleDiff		= settings.value("ropts-radialAngleDiff", 	45 * 3).toBool();
	radialLevelDiff		= settings.value("ropts-radialLevelDiff", 	100 / 3).toBool();
	radialLineWeight	= settings.value("ropts-radialLineWeight", 	200).toBool();

}

void MapRenderOptions::saveToQSettings()
{
	QSettings settings("wifisigmap");
	
	settings.setValue("ropts-cacheMapRender",	cacheMapRender);
	settings.setValue("ropts-showReadingMarkers",	showReadingMarkers);
	settings.setValue("ropts-multipleCircles",	multipleCircles);
	settings.setValue("ropts-fillCircles",		fillCircles);
	settings.setValue("ropts-radialCircleSteps",	radialCircleSteps);
	settings.setValue("ropts-radialLevelSteps",	radialLevelSteps);
	settings.setValue("ropts-radialAngleDiff",	radialAngleDiff);
	settings.setValue("ropts-radialLevelDiff",	radialLevelDiff);
	settings.setValue("ropts-radialLineWeight",	radialLineWeight);
}


void MapGraphicsScene::setRenderOpts(MapRenderOptions opts)
{
	m_renderOpts = opts;
	/*
	//Defaults:
	m_renderOpts.cacheMapRender     = true;
	m_renderOpts.showReadingMarkers = true;
	m_renderOpts.multipleCircles	= true;
	m_renderOpts.fillCircles	= true;
	m_renderOpts.radialCircleSteps	= 4 * 4 * 4;
	m_renderOpts.radialLevelSteps	= (int)(100 / .25);
	m_renderOpts.radialAngleDiff	= 45 * 3;
	m_renderOpts.radialLevelDiff	= 100 / 3;
	m_renderOpts.radialLineWeight	= 200;
	*/
	
	m_sigMapItem->setInternalCache(m_renderOpts.cacheMapRender);
	
	foreach(SigMapValue *val, m_sigValues)
		val->marker->setVisible(m_renderOpts.showReadingMarkers);
		
	m_renderOpts.saveToQSettings();
	
	triggerRender();
}


void MapGraphicsScene::setPixelsPerMeter(double m)
{
	m_pixelsPerMeter = m;
	m_pixelsPerFoot  = m / 3.28084;
}

void MapGraphicsScene::setPixelsPerFoot(double m)
{
	m_pixelsPerFoot  = m;
	m_pixelsPerMeter = m * 3.28084;
}

void MapGraphicsScene::setShowMyLocation(bool flag)
{
	m_showMyLocation = flag;
	QSettings("wifisigmap").setValue("showMyLocation", flag);
	if(!flag)
		m_userItem->setVisible(false); // will be shown on next scanFinished()
}

void MapGraphicsScene::setAutoGuessApLocations(bool flag)
{
	m_autoGuessApLocations = flag;
	QSettings("wifisigmap").setValue("autoGuessApLocations", flag);
	if(flag)
		updateApLocationOverlay(); // only updatd when adding signals, so update it here
	else
		m_apLocationOverlay->setVisible(false);
}

