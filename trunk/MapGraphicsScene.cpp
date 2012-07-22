#include "MapGraphicsScene.h"
#include "MapWindow.h"

#include "MapGraphicsView.h"
#include "LongPressSpinner.h"
#include "SigMapRenderer.h"

#ifdef Q_OS_ANDROID
#include <QSensor>
#include <QSensorReading>
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

static QString MapGraphicsScene_sort_apMac;

bool MapGraphicsScene_sort_SigMapValue_bySignal(SigMapValue *a, SigMapValue *b)
{
	QString apMac  = MapGraphicsScene_sort_apMac;
	if(!a || !b) return false;

	double va = a && a->hasAp(apMac) ? a->signalForAp(apMac) : 0.;
	double vb = b && b->hasAp(apMac) ? b->signalForAp(apMac) : 0.;
	return va < vb;

}

/* circle_circle_intersection() *
 * Determine the points where 2 circles in a common plane intersect.
 *
 * int circle_circle_intersection(
 *                                // center and radius of 1st circle
 *                                double x0, double y0, double r0,
 *                                // center and radius of 2nd circle
 *                                double x1, double y1, double r1,
 *                                // 1st intersection point
 *                                double *xi, double *yi,
 *                                // 2nd intersection point
 *                                double *xi_prime, double *yi_prime)
 *
 * This is a public domain work. 3/26/2005 Tim Voght
 *
 */
#include <stdio.h>
#include <math.h>

int circle_circle_intersection(double x0, double y0, double r0,
                               double x1, double y1, double r1,
                               double *xi, double *yi,
                               double *xi_prime, double *yi_prime)
{
  double a, dx, dy, d, h, rx, ry;
  double x2, y2;

  /* dx and dy are the vertical and horizontal distances between
   * the circle centers.
   */
  dx = x1 - x0;
  dy = y1 - y0;

  /* Determine the straight-line distance between the centers. */
  //d = sqrt((dy*dy) + (dx*dx));
  d = hypot(dx,dy); // Suggested by Keith Briggs

  /* Check for solvability. */
  if (d > (r0 + r1))
  {
    /* no solution. circles do not intersect. */
    qDebug() << "circle_circle_intersection: no intersect: d:"<<d<<", r0:"<<r0<<", r1:"<<r1;
    return 0;
  }
  if (d < fabs(r0 - r1))
  {
    /* no solution. one circle is contained in the other */
    qDebug() << "circle_circle_intersection: one in the other";
    return 0;
  }

  /* 'point 2' is the point where the line through the circle
   * intersection points crosses the line between the circle
   * centers.
   */

  /* Determine the distance from point 0 to point 2. */
  a = ((r0*r0) - (r1*r1) + (d*d)) / (2.0 * d) ;

  /* Determine the coordinates of point 2. */
  x2 = x0 + (dx * a/d);
  y2 = y0 + (dy * a/d);

  /* Determine the distance from point 2 to either of the
   * intersection points.
   */
  h = sqrt((r0*r0) - (a*a));

  /* Now determine the offsets of the intersection points from
   * point 2.
   */
  rx = -dy * (h/d);
  ry = dx * (h/d);

  /* Determine the absolute intersection points. */
  *xi = x2 + rx;
  *xi_prime = x2 - rx;
  *yi = y2 + ry;
  *yi_prime = y2 - ry;

  return 1;
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
	m_longPressTimer.setInterval(500);
	m_longPressTimer.setSingleShot(true);
	
	// Setup the longpress "progress counter" (display progress to user)
	connect(&m_longPressCountTimer, SIGNAL(timeout()), this, SLOT(longPressCount()));
	m_longPressCountTimer.setInterval(m_longPressTimer.interval() / 10);
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
	m_showMyLocation       = settings.value("showMyLocation", true).toBool();
	m_autoGuessApLocations = settings.value("autoGuessApLocations", true).toBool();

	// Set defualt pixels per foot
	setPixelsPerFoot(0.25/10);

	// Load last device/file used
	setDevice(settings.value("device", "").toString());

	// Load last render mode
	m_renderMode = (RenderMode)settings.value("rendermode", (int)DEFAULT_RENDER_MODE).toInt();

	// Ask MapRenderOpts to load itself from QSettings
	m_renderOpts.loadFromQSettings();
	
	qDebug() << "MapGraphicsScene: Setup and ready to go.";
	
	QTimer::singleShot(1000, this, SLOT(debugTest()));
	
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
	markerGroup = addDropShadow(markerGroup, (double)(markerGroup.width()/2));
		
	//markerGroup.save("apMarkerDebug.png");
	
	markerGroup = tinted(markerGroup, baseColorForAp(mac));

	QGraphicsPixmapItem *item = addPixmap(QPixmap::fromImage(markerGroup));
	item->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
	
	double w2 = (double)(markerGroup.width())/2.;
	double h2 = (double)(markerGroup.height())/2.;
	item->setPos(point);
	item->setOffset(-w2,-h2);
	
// 	item->setFlag(QGraphicsItem::ItemIsMovable);
// 	item->setFlag(QGraphicsItem::ItemSendsGeometryChanges);
// 	item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
	item->setZValue(99);
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

QLineF calcIntersect(QPointF p0, double r0, QPointF p1, double r1)
{ 
	// Based following code on http://paulbourke.net/geometry/2circle/
	// Test cases under "First calculate the distance d between the center of the circles. d = ||P1 - P0||."
	//QPointF delta = p1 - p0;
	//ouble d = sqrt(delta.x()*delta.x() + delta.y()*delta.y());
	double d = QLineF(p1,p0).length();
	
	qDebug() << "calcIntersect(): r0:"<<r0<<", r1:"<<r1<<", p0:"<<p0<<", p1:"<<p1<<", d:"<<d;
// 	qDebug() << "\t ratio0 * apMacToSignal[ap0]:"<<ratio0<<apMacToSignal[ap0];
// 	qDebug() << "\t ratio1 * apMacToSignal[ap1]:"<<ratio1<<apMacToSignal[ap1];
// 	
	if(d > r0 + r1)
	{
		// If d > r0 + r1 then there are no solutions, the circles are separate.
		qDebug() << "calcIntersect(): d > r0 + r1, there are no solutions, the circles are separate.";
		//m_userItem->setVisible(false);
		return QLineF();
	}
	
	if(d < fabs(r0 - r1))
	{
		// If d < |r0 - r1| then there are no solutions because one circle is contained within the other.
		qDebug() << "calcIntersect(): d < |r0 - r1|, there are no solutions because one circle is contained within the other.";
		//m_userItem->setVisible(false);
		return QLineF();
	}
	
	if(d == 0. && r0 == r1)
	{
		// If d = 0 and r0 = r1 then the circles are coincident and there are an infinite number of solutions.
		qDebug() << "calcIntersect(): d = 0 and r0 = r1, the circles are coincident and there are an infinite number of solutions.";
		//m_userItem->setVisible(false);
		return QLineF();
	}
	
// 	// a = (r0^2 - r1^2 + d^2 ) / (2 d)
// 	double a = ( (r0*r0) - (r1*r1) + (d*d) ) / (2 * d);
// 	
// 	// P2 = P0 + a ( P1 - P0 ) / d
// 	QPointF p2 = p0 +  a * (p1 - p0) / d;
// 	
// 	// x3 = x2 +- h ( y1 - y0 ) / d
// 	// y3 = y2 -+ h ( x1 - x0 ) / d
// 	double y3 = p2.y() + 

	/// From http://2000clicks.com/mathhelp/GeometryConicSectionCircleIntersection.aspx:
	
	// d^2 = (xB-xA)^2 + (yB-yA)^2 
	// K = (1/4)sqrt(((rA+rB)^2-d^2)(d^2-(rA-rB)^2))
	// x = (1/2)(xB+xA) + (1/2)(xB-xA)(rA^2-rB^2)/d^2 ± 2(yB-yA)K/d^2 
	// y = (1/2)(yB+yA) + (1/2)(yB-yA)(rA^2-rB^2)/d^2 ± -2(xB-xA)K/d^2
	
	/// K = (1/4)sqrt(((rA+rB)^2-d^2)(d^2-(rA-rB)^2))
	double K = .25 * sqrt( ((r0+r1)*(r0+r1) - d*d) * (d*d - (r0-r1) * (r0-r1)) );
	
	/// x = (1/2)(xB+xA) + (1/2)(xB-xA)(rA^2-rB^2)/d^2 ± 2(yB-yA)K/d^2
	double x1 = .5 * (p1.x()+p0.x()) + .5 * (p1.x()-p0.x()) * (r0*r0-r1*r1) / (d*d) + 2 * (p1.y()-p0.y()) * K / (d*d);
	double x2 = .5 * (p1.x()+p0.x()) + .5 * (p1.x()-p0.x()) * (r0*r0-r1*r1) / (d*d) - 2 * (p1.y()-p0.y()) * K / (d*d);
	
	/// y = (1/2)(yB+yA) + (1/2)(yB-yA)(rA^2-rB^2)/d^2 ± -2(xB-xA)K/d^2
	double y1 = .5 * (p1.y()+p0.y()) + .5 * (p1.y()-p0.y()) * (r0*r0-r1*r1) / (d*d) + 2 * (p1.x()-p0.x()) * K / (d*d);
	double y2 = .5 * (p1.x()+p0.y()) + .5 * (p1.y()-p0.y()) * (r0*r0-r1*r1) / (d*d) - 2 * (p1.x()-p0.x()) * K / (d*d);
	
	return QLineF(x1,y1,x2,y2);
}

double dist2(const QPointF &p0, const QPointF &p1)
{
	QPointF delta = p1 - p0;
	return delta.x()*delta.x() + delta.y()*delta.y();
}

double dist23(const QPointF &p0, const QPointF &p1, const QPointF &p2)
{
	QPointF delta = p2 - p1 - p0; 
	return delta.x()*delta.x() + delta.y()*delta.y();
}

// double operator-(const QPointF &a, const QPointF &b)
// {
// 	return sqrt(dist2(a,b));
// }

QPointF operator*(const QPointF&a, const QPointF& b) { return QPointF(a.x()*b.x(), a.y()*b.y()); }

void MapGraphicsScene::scanFinished(QList<WifiDataResult> results)
{
	
	QPointF realPoint;

// 	static bool firstScan = true;
// 	if(!firstScan)
// 		return;
// 		
// 	firstScan = false;
	
	//qDebug() << "MapGraphicsScene::scanFinished(): currentThreadId:"<<QThread::currentThreadId();
	
	// Sort the results high-low
	qSort(results.begin(), results.end(), MapGraphicsScene_sort_WifiDataResult);
	
	m_lastScanResults = results;
	
	foreach(WifiDataResult result, results)
	{
		MapApInfo *info = apInfo(result.mac);
		if(info->essid.isEmpty())
			info->essid = result.essid;
	}
		
	update(); // allow HUD to update
	
	updateUserLocationOverlay();
	//updateApLocationOverlay();
}

void MapGraphicsScene::updateUserLocationOverlay()
{
	if(!m_showMyLocation)
	{
		m_userItem->setVisible(false);
		return;
	}

	// We still can estimate location without any signal readings - we'll just use default loss factors
	/*
	if(m_sigValues.isEmpty())
		return;
	*/

	//return;

	
	QSize origSize = m_bgPixmap.size();
	if(origSize.width() * 2 * origSize.height() * 2 * 3 > 128*1024*1024)
	{
		qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): Size too large, not updating: "<<origSize;
		return;
	}


	QImage image(QSize(origSize.width()*2,origSize.height()*2), QImage::Format_ARGB32_Premultiplied);
	QPointF offset = QPointF();
	
	memset(image.bits(), 0, image.byteCount());
	//image.fill(QColor(255,0,0,50).rgba());
	
	
	/// JUST for debugging
	// 	realPoint = m_sigValues.last()->point;
	// 	results   = m_sigValues.last()->scanResults;
	//
	
	QPointF userLocation  = QPointF(-1000,-1000);

	// Need at least two *known* APs registered on the map - with out that, we don't even need to check the results
	if(m_apInfo.values().size() < 2)
	{
		qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): Less than two APs observed, unable to guess user location";
		return;
	}
	
	// Build a hash table of MAC->Signal for eash access and a list of MACs in decending order of signal strength
	QStringList apsVisible;
	QHash<QString,double> apMacToSignal;
	QHash<QString,int> apMacToDbm;
	foreach(WifiDataResult result, m_lastScanResults)
	{
		MapApInfo *info = apInfo(result.mac);
		
		//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): Checking result: "<<result;
		if(!apMacToSignal.contains(result.mac) &&
		  (!info->point.isNull() || !info->locationGuess.isNull()))
		{
			// Use location guess as 'point' for calculations below
			if(!info->marked && info->locationGuess.isNull())
				info->point = info->locationGuess;
			
			apsVisible << result.mac;
			apMacToSignal.insert(result.mac, result.value);

			#ifdef OPENCV_ENABLED
			// Use Kalman to filter the dBm value
			//MapApInfo *info = apInfo(result.mac);
			info->kalman.predictionUpdate(result.dbm, 0);

			float value = (float)result.dbm, tmp = 0;
			info->kalman.predictionReport(value, tmp);
			apMacToDbm.insert(result.mac, (int)value);
			result.dbm = (int)value;
			
			#else

			apMacToDbm.insert(result.mac, result.dbm);
			
			#endif

			//qDebug() << "[apMacToSignal] mac:"<<result.mac<<", val:"<<result.value<<", dBm:"<<result.dbm;
		}
	}
	
	// Need at least two APs *visible* in results *AND* marked on the MAP
	// (apsVisible only contains APs marked on the map AND in the latest scan results) 
	if(apsVisible.size() < 2)
	{
		//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): Less than two known APs marked AND visble, unable to guess user location";
		m_mapWindow->setStatusMessage("Need at least 2 APs visible to calculate location", 2000);
		return;
	}

	//m_mapWindow->setStatusMessage("Calculating location...", 1000);
	//qDebug() << "\n\nMapGraphicsScene::updateUserLocationOverlay(): Updating Location...";
/*	
	// For each AP, average the ratio of pixels-to-signalLevel by averaging the location of every reading for that AP in relation to the marked location of the AP.
	// This is used to calculate the radius of the APs coverage.
	QHash<QString,int>    apRatioCount;
	QHash<QString,double> apRatioSums;
	QHash<QString,double> apRatioAvgs;
	
	
	foreach(QString apMac, apsVisible)
	{
		QPointF center = apInfo(apMac)->point;
		
		double maxDistFromCenter = -1;
		double maxSigValue = 0.0;
		foreach(SigMapValue *val, m_sigValues)
		{
			if(val->hasAp(apMac))
			{
				double dx = val->point.x() - center.x();
				double dy = val->point.y() - center.y();
				double distFromCenter = sqrt(dx*dx + dy*dy);
				if(distFromCenter > maxDistFromCenter)
				{
					maxDistFromCenter = distFromCenter;
					maxSigValue = val->signalForAp(apMac);
				}
			}
		}
		
		apRatioAvgs[apMac] = maxDistFromCenter;// / maxSigValue;
		qDebug() << "[ratio calc] "<<apMac<<": maxDistFromCenter:"<<maxDistFromCenter<<", macSigValue:"<<maxSigValue<<", ratio:"<<apRatioAvgs[apMac]; 
	}
			
	
// 	foreach(SigMapValue *val, m_sigValues)
// 	{
// 		//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [ratio calc] val:"<<val; 
// 		
// 		// Remember, apsVisible has all the APs *visible* in this scan result *AND* marked in the map 
// 		foreach(QString apMac, apsVisible)
// 		{
// 			//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [ratio calc] apMac:"<<apMac;
// 			if(val->hasAp(apMac) &&
// 			  !apInfo(apMac)->point.isNull()) // TODO Since apsVisible are only APs marked, do we need to test this still?
// 			{
// 				QPointF delta = apInfo(apMac)->point - val->point;
// 				double d = sqrt(delta.x()*delta.x() + delta.y()*delta.y());
// 				//double dist = d;
// 				d = d / val->signalForAp(apMac);
// 				
// 				//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [ratio calc] \t ap:"<<apMac<<", d:"<<d<<", val:"<<val->signalForAp(apMac)<<", dist:"<<dist; 
// 				
// 				// Incrememnt counters/sums
// 				if(!apRatioSums.contains(apMac))
// 				{
// 					apRatioSums.insert(apMac, d);
// 					apRatioCount.insert(apMac, 1);
// 				}
// 				else
// 				{
// 					apRatioSums[apMac] += d;
// 					apRatioCount[apMac] ++;
// 				}
// 			}
// 		}
// 	}
// 	
// 	// Average the values found above to determine the pixel-signal ratio
// 	foreach(QString key, apRatioSums.keys())
// 	{
// 		apRatioAvgs[key] = apRatioSums[key] / apRatioCount[key];
// 		//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [ratio calc] final avg for mac:"<<key<<", avg:"<<apRatioAvgs[key]<<", count:"<<apRatioCount[key]<<", sum:"<<apRatioSums[key];
// 	}
	
	
	// apsVisible implicitly is sorted strong->weak signal
	QString ap0 = apsVisible[0];
	QString ap1 = apsVisible[1];
	QString ap2 = apsVisible.size() > 2 ? apsVisible[2] : "";
	
	//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): ap0: "<<ap0<<", ap1:"<<ap1; 
	
	double ratio0 = 0;
	double ratio1 = 0;
	double ratio2 = 0;
	
	if(apRatioAvgs.isEmpty())
	{
		qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): Need at least one reading for ANY marked AP to establish even a 'best guess' ratio";
		return;
	}
	else
	{
		// It's possible two APs are VISIBLE and MARKED, *but* they don't have a signal reading for one of the APs, so we guess by using the ratio of the first AP in the apRatioAvgs hash 
		if(!apRatioAvgs.contains(ap0))
		{
			ratio0 = apRatioAvgs[apRatioAvgs.keys().first()];
			qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): Need at least one reading for "<<ap0<<" to establish correct dBm to pixel ratio, cheating by using first";
		}
		else
			ratio0 = apRatioAvgs[ap0];
			
		if(!apRatioAvgs.contains(ap1))
		{
			ratio1 = apRatioAvgs[apRatioAvgs.keys().first()];
			qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): Need at least one reading for "<<ap1<<" to establish correct dBm to pixel ratio, cheating by using first";
		}
		else
			ratio1 = apRatioAvgs[ap1];
			
		if(!apRatioAvgs.contains(ap2))
		{
			ratio2 = apRatioAvgs[apRatioAvgs.keys().first()];
			qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): Need at least one reading for "<<ap2<<" to establish correct dBm to pixel ratio, cheating by using first";
		}
		else
			ratio2 = apRatioAvgs[ap2];
	}
	
	
	// this is safe to not check apInfo() for null ptr, since apInfo() auto-creates a MapApInfo object for any previously-unknown macs
	QPointF p0 = apInfo(ap0)->point;
	QPointF p1 = apInfo(ap1)->point;
	QPointF p2 = !ap2.isEmpty() ? apInfo(ap2)->point : QPointF();
	//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): p0: "<<p0<<", p1:"<<p1;
	
	double r0 = apMacToSignal[ap0] * ratio0;
	double r1 = apMacToSignal[ap1] * ratio1;
	double r2 = apMacToSignal[ap2] * ratio2;
	
	qDebug() << "[radius] "<<ap0<<": r0:"<<r0<<", ratio0:"<<ratio0<<", signal:"<<apMacToSignal[ap0];
	qDebug() << "[radius] "<<ap1<<": r1:"<<r1<<", ratio1:"<<ratio1<<", signal:"<<apMacToSignal[ap1];
	qDebug() << "[radius] "<<ap2<<": r2:"<<r2<<", ratio2:"<<ratio2<<", signal:"<<apMacToSignal[ap2];
	
	QLineF line  = calcIntersect(p0,r0, p1,r1);
	QLineF line2 = calcIntersect(p1,r1, p2,r2);
	QLineF line3 = calcIntersect(p2,r2, p0,r0);
	
	// Make an image to contain x1,y1 and x2,y2, or at least the user's icon
	QRectF itemWorldRect = QRectF(line.p1(), QSizeF(
		line.p1().x() - line.p2().x(), 
		line.p1().y() - line.p2().y()))
			.normalized();
	
	//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): Calculated (x1,y1):"<<line.p1()<<", (x2,y2):"<<line.p2()<<", itemWorldRect:"<<itemWorldRect<<", center:"<<itemWorldRect.center();
	//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): #2 debug: "<<ap2<<": "<<p2<<", radius: "<<r2;
	
	// Attempt http://stackoverflow.com/questions/9747227/2d-trilateration
	
	// Define:
	// ex,x = (P2x - P1x) / sqrt((P2x - P1x)2 + (P2y - P1y)2)
	// ex,y = (P2y - P1y) / sqrt((P2x - P1x)2 + (P2y - P1y)2)

	// Steps:
	// 1. ex = (P2 - P1) / ‖||P2 - P1||
	QPointF vex = (p1 - p0) / sqrt(dist2(p1, p0));
	// 2. i = ex(P3 - P1)
	QPointF vi  = vex * (p2 - p1);
	// ey = (P3 - P1 - i · ex) / ||‖P3 - P1 - i · ex||
	QPointF ey = (p2 - p1 - (vi * vex)) / dist23(p2,p0,vi*vex);
	// d = ||‖P2 - P1‖||
	double d = sqrt(dist2(p1,p0));
	// j = ey(P3 - P1)
	QPointF vj = vex * (p1 - p0);
	// x = (r12 - r22 + d2) / 2d
	double x = (r0*r0 - r1*r1 + d*d)/(2*d);(void)x;//ignore warning
	// TODO: This line doesn't make sense: (type conversion?)
	// y = (r12 - r32 + i2 + j2) / 2j - ix / j
	//double y = (r0*r0 - r2*r2 + vi*vi + vj*vj) / (2*vj - vix/vij);
	
	
	// That didn't work, so let's tri triangulation
	
	
	
// 	  C
// 	  *
// 	  |`.
// 	 b|  `. a
// 	  |    `.
// 	  |      `.
// 	A *--------* B
// 	      c
		
	
	QLineF apLine(p1,p0);
	
	QLineF realLine(p0,realPoint);
	double realAngle = realLine.angle();
	
	QLineF realLine2(p1,realPoint);
	double realAngle2 = realLine2.angle();
	
// 	double la = r0; //realLine2.length(); //r0;
// 	double lb = r1; //realLine.length(); //r1;
	double lc = apLine.length(); //sqrt(dist2(p1,p0));
	
	
// 	double fA = 1./(10.*n);
// 	double fB = pTx - pRx + gTx + gRx;
// 	double fC = -Xa + (20.*log10(m)) - (20.*log10(4.*Pi));
// 	
	
	QPointF lossFactor2 = deriveObservedLossFactor(ap2);	
	QPointF lossFactor1 = deriveObservedLossFactor(ap1);
	QPointF lossFactor0 = deriveObservedLossFactor(ap0);
	
	// Store newly-derived loss factors into apInfo() for use in dBmToDistance()
	apInfo(ap2)->lossFactor = lossFactor2;
	apInfo(ap1)->lossFactor = lossFactor1;
	apInfo(ap0)->lossFactor = lossFactor0;
	
// 	qDebug() << "[formula] codified comparrison (ap1): "<<dBmToDistance(apMacToDbm[ap1], ap1);
// 	qDebug() << "[formula] codified comparrison (ap0): "<<dBmToDistance(apMacToDbm[ap0], ap0);
	
	double lb = dBmToDistance(apMacToDbm[ap1], ap1) * m_pixelsPerMeter;
	double la = dBmToDistance(apMacToDbm[ap0], ap0) * m_pixelsPerMeter;
	
	r2 = dBmToDistance(apMacToDbm[ap2], ap2) * m_pixelsPerMeter;
	
	qDebug() << "[dump1] "<<la<<lb<<lc;
	qDebug() << "[dump2] "<<realAngle<<realAngle2<<apLine.angle();
	qDebug() << "[dump3] "<<p0<<p1<<realPoint;
	qDebug() << "[dump4] "<<realLine.angleTo(apLine)<<realLine2.angleTo(apLine)<<realLine2.angleTo(realLine);
		
// 	la= 8;
// 	lb= 6;
// 	lc= 7;
	
// 	la = 180;
// 	lb = 238;
// 	lc = 340;
	
	// cos A = (b2 + c2 - a2)/2bc
	// cos B = (a2 + c2 - b2)/2ac
	// cos C = (a2 + b2 - c2)/2ab
	double cosA = (lb*lb + lc*lc - la*la)/(2*lb*lc);
	double cosB = (la*la + lc*lc - lb*lb)/(2*la*lc);
	double cosC = (la*la + lb*lb - lc*lc)/(2*la*lb);
	double angA = acos(cosA)* 180.0 / Pi;
	double angB = acos(cosB)* 180.0 / Pi;
	double angC = acos(cosC)* 180.0 / Pi;
	//double angC = 180 - angB - angA;
			
	
	QLineF userLine(p1,QPointF());
	QLineF userLine2(p0,QPointF());
	
	//double userLineAngle = 90+45-angB+apLine.angle()-180;
	//userLineAngle *= -1;
	//userLine.setAngle(angA + apLine.angle() + 90*3);
	userLine.setAngle(angA + apLine.angle());
// 	userLine.setLength(r1);
	userLine.setLength(lb);
	
	userLine2.setAngle(angA + angC + apLine.angle());
// 	userLine2.setLength(r0);
	userLine2.setLength(la);
	
	double realAngle3 = realLine2.angleTo(realLine);
	
	double errA = realAngle  - angA;
	double errB = realAngle2 - angB;
	double errC = realAngle3 - angC;
	
	qDebug() << "Triangulation: ang(A-C):"<<angA<<angB<<angC<<", cos(A-C):"<<cosA<<cosB<<cosC;
	qDebug() << "Triangulation: err(A-C):"<<errA<<errB<<errC;
	qDebug() << "Triangulation: abs(A-C):"<<realAngle<<realAngle2<<apLine.angle();
	qDebug() << "Triangulation: realAngle:"<<realAngle<<"/"<<realLine.length()<<", realAngle2:"<<realAngle2<<"/"<<realLine2.length();
	qDebug() << "Triangulation: apLine.angle:"<<apLine.angle()<<", line1->line2 angle:"<<userLine.angleTo(userLine2)<<", realAngle3:"<<realAngle3;
	
	//QPointF calcPoint = userLine.p2();
	
	QPointF calcPoint = triangulate(ap0, apMacToDbm[ap0], 
					ap1, apMacToDbm[ap1]);
	
	userLine  = QLineF(p1, calcPoint);
	userLine2 = QLineF(p0, calcPoint);
	
*/	
	
/*
	QImage image(
		(int)qMax((double)itemWorldRect.size().width(),  64.) + 10,
		(int)qMax((double)itemWorldRect.size().height(), 64.) + 10,
		QImage::Format_ARGB32_Premultiplied);
*/
	QRect picRect = m_sigMapItem->picture().boundingRect();
//		/*QPointF */offset = picRect.topLeft();
		
// 	QImage image(picRect.size(), QImage::Format_ARGB32_Premultiplied);
// 	QPainter p(&image);
// 	p.translate(-offset);
		
// 		QSize origSize = m_bgPixmap.size();
// 		QImage image(origSize, QImage::Format_ARGB32_Premultiplied);
//		offset = QPointF();
		
//		memset(image.bits(), 0, image.byteCount());
	QPainter p(&image);
	p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
	p.translate(origSize.width()/2,origSize.height()/2);
	
	//QPainter p(&image);
	
	//p.setPen(Qt::black);
		
//	QRectF rect(QPointF(0,0),itemWorldRect.size());
// 	QRectF rect = itemWorldRect;
// 	QPointF center = rect.center();
	
	#ifdef Q_OS_ANDROID
	QString size = "64x64";
	#else
	QString size = "32x32";
	#endif
	
	double penWidth = 15.0;
	
	QHash<QString,bool> drawnFlag;
	QHash<QString,int> badLossFactor;
	
	QVector<QPointF> userPoly;
	
	QPointF avgPoint(0.,0.);
	int count = 0;
	
	int numAps = apsVisible.size();
	for(int i=0; i<numAps; i++)
	{
		QString ap0 = apsVisible[i];
		QString ap1 = (i < numAps - 1) ? apsVisible[i+1] : apsVisible[0];
		
		QPointF p0 = apInfo(ap0)->point;
		QPointF p1 = apInfo(ap1)->point;
		
		QPointF calcPoint = triangulate(ap0, apMacToDbm[ap0],
						ap1, apMacToDbm[ap1]);
		
		if(isnan(calcPoint.x()) || isnan(calcPoint.y()))
		{
			if(!badLossFactor.contains(ap0))
				badLossFactor[ap0] = 0;
			else
				badLossFactor[ap0] = badLossFactor[ap0] + 1;
			
			if(!badLossFactor.contains(ap1))
				badLossFactor[ap1] = 0;
			else
				badLossFactor[ap1] = badLossFactor[ap1] + 1;
				
			qDebug() << "\t NaN: "<<badLossFactor[ap0]<<" / "<<badLossFactor[ap1];
		}
		else
		{
			avgPoint += calcPoint;
			count ++;
		}
	}
	
// 	avgPoint.setX( avgPoint.x() / count );
// 	avgPoint.setY( avgPoint.y() / count );
	avgPoint /= count;
	
	foreach(QString ap, apsVisible)
	{
		if(badLossFactor[ap] > 0)
		{
			MapApInfo *info = apInfo(ap);
			
			double avgMeterDist = QLineF(avgPoint, info->point).length() / m_pixelsPerMeter;
			double absLossFactor = deriveLossFactor(ap, apMacToDbm[ap], avgMeterDist/*, gRx*/);

			if(isnan(absLossFactor))
			{
				//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap<<": Unable to correct, received NaN loss factor, avgMeterDist:"<<avgMeterDist<<", absLossFactor:"<<absLossFactor;
			}
			else
			{
				QPointF lossFactor = info->lossFactor;
				if(apMacToDbm[ap] > info->shortCutoff)
					lossFactor.setY(absLossFactor);
				else
					lossFactor.setX(absLossFactor);

				info->lossFactor = lossFactor;

				//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap<<": Corrected loss factor:" <<lossFactor<<", avgMeterDist:"<<avgMeterDist<<", absLossFactor:"<<absLossFactor;
			}
		}
	}

	QPointF avgPoint2(0.,0.);
	count = 0;

	QList<QPointF> goodPoints;
	QList<QPointF> badPoints;
	QLineF firstSet;

	QStringList pairsTested;
	
// 	int numAps = apsVisible.size();
//	for(int i=1; i<numAps; i++)
	bool needFirstGoodPoint = false;
	for(int i=0; i<numAps; i++)
	{
// 		QString ap0 = apsVisible[i];
// 		QString ap1 = (i < numAps - 1) ? apsVisible[i+1] : apsVisible[0];

// 		QString ap0 = apsVisible[0];
// 		QString ap1 = apsVisible[i]; //(i < numAps - 1) ? apsVisible[i+1] : apsVisible[0];
		
		for(int j=0; j<numAps; j++)
		{
		
			QString key = QString("%1%2").arg(i<j?i:j).arg(i<j?j:i);
			if(i == j || pairsTested.contains(key))
			{
				//qDebug() << "\t not testing ("<<i<<"/"<<j<<"): key:"<<key;
				continue;
			}
			
			//qDebug() << "\t testing ("<<i<<"/"<<j<<"): key:"<<key;
			
			pairsTested << key;
			
			QString ap0 = apsVisible[i];
			QString ap1 = apsVisible[j];
			
	
			MapApInfo *info0 = apInfo(ap0);
			MapApInfo *info1 = apInfo(ap1);
			
			QPointF p0 = info0->point;
			QPointF p1 = info1->point;
			
			QPointF calcPoint = triangulate(ap0, apMacToDbm[ap0],
							ap1, apMacToDbm[ap1]);
			
			// We assume triangulate() already stored drived loss factor into apInfo()
			double r0 = dBmToDistance(apMacToDbm[ap0], ap0) * m_pixelsPerMeter;
			double r1 = dBmToDistance(apMacToDbm[ap1], ap1) * m_pixelsPerMeter;
	
			double dist = QLineF(p1,p0).length();
	
			if(dist > r0 + r1)
			{
				// If d > r0 + r1 then there are no solutions, the circles are separate.
				
				// Logic tells us that since both signals were observed by the user in the same reading, they must intersect,
				// therefore the solution given by dBmToDistance() must be wrong.
	
				// Therefore, we will adjust the lossFactor for these APs inorder to provide
				// an intersection by allocation part of the error to each AP
	
				double errorDist = dist - (r0 + r1);
				
				double correctR0 = (r0 + errorDist*.6) / m_pixelsPerMeter;
				double correctR1 = (r1 + errorDist*.6) / m_pixelsPerMeter;
				
				double absLossFactor0 = deriveLossFactor(ap0, apMacToDbm[ap0], correctR0 /*, gRx*/);
				double absLossFactor1 = deriveLossFactor(ap1, apMacToDbm[ap1], correctR1 /*, gRx*/);
	
				if(isnan(absLossFactor0))
				{
					//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<": Unable to correct (d>r0+r1), received NaN loss factor, corretR0:"<<correctR0<<", absLossFactor0:"<<absLossFactor0;
				}
				else
				{
					QPointF lossFactor = info0->lossFactor;
					if(apMacToDbm[ap0] > info0->shortCutoff)
						lossFactor.setY(absLossFactor0);
					else
						lossFactor.setX(absLossFactor0);
	
					info0->lossFactor = lossFactor;
	
					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<": Corrected loss factor for ap0:" <<lossFactor<<", correctR0:"<<correctR0<<", absLossFactor0:"<<absLossFactor0;
				}
	
				if(isnan(absLossFactor1))
				{
					//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap1<<": Unable to correct (d>r0+r1), received NaN loss factor, corretR1:"<<correctR0<<", absLossFactor0:"<<absLossFactor0;
				}
				else
				{
					QPointF lossFactor = info1->lossFactor;
					if(apMacToDbm[ap1] > info1->shortCutoff)
						lossFactor.setY(absLossFactor1);
					else
						lossFactor.setX(absLossFactor1);
	
					info0->lossFactor = lossFactor;
	
					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap1<<": Corrected loss factor for ap1:" <<lossFactor<<", correctR1:"<<correctR1<<", absLossFactor:"<<absLossFactor1;
				}
	
				// Recalculate distances
				r0 = dBmToDistance(apMacToDbm[ap0], ap0) * m_pixelsPerMeter;
				r1 = dBmToDistance(apMacToDbm[ap1], ap1) * m_pixelsPerMeter;
	
				if(dist > r0 + r1)
				{
					// Distance still wrong, so force-set the proper distance
					double errorDist = dist - (r0 + r1);
	
					r0 += errorDist * .6; // overlay a bit by using .6 instead of .5
					r1 += errorDist * .6;
	
					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): force-corrected the radius: "<<r0<<r1<<", errorDist: "<<errorDist;
				}
				
			}
	
	
			QColor color0 = baseColorForAp(ap0);
			QColor color1 = baseColorForAp(ap1);
	
	
			// Render the estimated circles covered by these APs
			if(!drawnFlag.contains(ap0))
			{
				drawnFlag.insert(ap0, true);
	
				p.setPen(QPen(color0, penWidth));
	
				if(isnan(r0))
					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<"->"<<ap1<<": - Can't render ellipse p0/r0 - radius 0 is NaN";
				else
					p.drawEllipse(p0, r0, r0);
			}
	
			if(!drawnFlag.contains(ap1))
			{
				drawnFlag.insert(ap1, true);
	
				p.setPen(QPen(color1, penWidth));
	
				if(isnan(r1))
					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<"->"<<ap1<<": - Can't render ellipse p0/r0 - radius 1 is NaN";
				else
					p.drawEllipse(p1, r1, r1);
	
			}
	
			//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [circle:pre] i:"<<i<<", r0:"<<r0<<", r1:"<<r1;
			
			// The revised idea here is this:
			// Need at least three APs to find AP with intersection of circles:
			// Get the two lines the APs intersect
			// Store the first two points at into firstSet
			// Next set, the next two points compared to first two - the two closest go into the goodPoints set, the rejected ones go into badPoints
			// From there, the next (third) AP gets compared to goodPoints - the point closest goes into goodPoints, etc
			// At end, good points forms the probability cluster of where the user probably is
			
			//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [circle:pre] goodPoints:"<<goodPoints<<", idx:"<<i<<", numAps:"<<numAps;
			
			QPointF goodPoint;
			
			//QLineF line  = calcIntersect(p0, r0, p1, r1);
			//QLineF line = triangulate2(ap0, apMacToDbm[ap0],
			//			   ap1, apMacToDbm[ap1]);
			double xi, yi, xi_prime, yi_prime;
			int ret = circle_circle_intersection(
						p0.x(), p0.y(), r0,
						p1.x(), p1.y(), r1,
						&xi, &yi,
						&xi_prime, &yi_prime);
			if(!ret)
			{
				qDebug() << "triangulate2(): circle_circle_intersection() returned 0";
				continue;
			}
	
			QLineF line(xi, yi, xi_prime, yi_prime);
	
			
			if(line.isNull())
			{
				continue;
			}
			else
			{
				p.setPen(QPen(Qt::gray, penWidth));
				p.drawLine(line);
				
				if(goodPoints.isEmpty())
				{
					if(firstSet.isNull())
					{
						firstSet = line;
						// If only intersecting two APs, then just just give the center of the line.
						// Otherwise, leave blank and let the code below handle it the first time
						goodPoint = numAps > 2 ? QPointF() : (line.p1() + line.p2()) / 2;
						
						//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [circle:step1] firstSet:"<<firstSet<<", goodPoint:"<<goodPoint<<", numAps:"<<numAps;
					}
					else
					{
						// This is the second AP intersection - here we compare both points in both lines
						// The two points closest together are the 'good' points
						double min = 0;
						QLineF good;
						QLineF bad;
						
						double len11 = QLineF(firstSet.p1(), line.p1()).length();
						{
							min = len11;
							good = QLineF(firstSet.p1(), line.p1());
							bad  = QLineF(firstSet.p2(), line.p2());
						}
						
						double len12 = QLineF(firstSet.p1(), line.p2()).length();
						if(len12 < min)
						{
							min = len12;
							good = QLineF(firstSet.p1(), line.p2());
							bad  = QLineF(firstSet.p2(), line.p1());
						}
						
						double len21 = QLineF(firstSet.p2(), line.p1()).length();
						if(len21 < min)
						{
							min = len21;
							good = QLineF(firstSet.p2(), line.p1());
							bad  = QLineF(firstSet.p1(), line.p2());
						}
						
						double len22 = QLineF(firstSet.p2(), line.p2()).length();
						if(len22 < min)
						{
							min = len22;
							good = QLineF(firstSet.p2(), line.p2());
							bad  = QLineF(firstSet.p1(), line.p1());
						}
						
						goodPoints << good.p1() << good.p2();
						badPoints  << bad.p1()  << bad.p2();
						
						goodPoint = good.p2();
						
						//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [circle:step2] firstSet:"<<firstSet<<", line:"<<line<<", goodPoints:"<<goodPoints;
					}
				}
				else
				{
					// Calculate avg distance for both intersection points,
					// then the point with the smallest avg 'good' distance is chosen as a good point, the other one is rejected
					double goodSum1=0, goodSum2=0;
					foreach(QPointF good, goodPoints)
					{
						goodSum1 += QLineF(good, line.p1()).length();
						goodSum2 += QLineF(good, line.p2()).length();
					}
					
					double count = (double)goodPoints.size();
					double goodAvg1 = goodSum1 / count,
					goodAvg2 = goodSum2 / count;
					
					if(goodAvg1 < goodAvg2)
					{
						qDebug() <<
						goodPoints << line.p1();
						badPoints  << line.p2();
						
						goodPoint = line.p1();
						
						//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [circle:step3] 1<2 ("<<goodAvg1<<":"<<goodAvg2<<"): line was: "<<line<<", goodPoints:"<<goodPoints;
					}
					else
					{
						goodPoints << line.p2();
						badPoints  << line.p1();
						
						goodPoint = line.p2();
						
						//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [circle:step3] 2<1 ("<<goodAvg1<<":"<<goodAvg2<<"): line was: "<<line<<", goodPoints:"<<goodPoints;
					}
				}
			}
			
			if(goodPoint.isNull())
			{
				needFirstGoodPoint = true;
			}
			else
			{
				if(needFirstGoodPoint)
				{
					needFirstGoodPoint = false;
					
					QPointF tmpPoint = goodPoints.first();
					
					userPoly << tmpPoint;
					
					//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<"->"<<ap1<<": Point:" <<tmpPoint << " (from last intersection)";
					
					
					p.save();
					
		// 			p.setPen(QPen(color0, penWidth));
		// 			p.drawLine(p0, calcPoint);
		// 			
		// 			p.setPen(QPen(color1, penWidth));
		// 			p.drawLine(p1, calcPoint);
					
					p.setPen(QPen(Qt::gray, 3.));
					p.setBrush(QColor(0,0,0,127));
					p.drawEllipse(tmpPoint, 10, 10);
					
					p.restore();
					
					avgPoint2 += tmpPoint;
					count ++;
				}
					
				calcPoint = goodPoint;
				
				if(isnan(calcPoint.x()) || isnan(calcPoint.y()))
				{
				//	qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<"->"<<ap1<<": - Can't render ellipse - calcPoint is NaN";
				}
				else
				{
					//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<"->"<<ap1<<": Point:" <<calcPoint;
					//qDebug() << "\t color0:"<<color0.name()<<", color1:"<<color1.name()<<", r0:"<<r0<<", r1:"<<r1<<", p0:"<<p0<<", p1:"<<p1;
					
					userPoly << goodPoint;
					
					p.save();
					
		// 			p.setPen(QPen(color0, penWidth));
		// 			p.drawLine(p0, calcPoint);
		// 			
		// 			p.setPen(QPen(color1, penWidth));
		// 			p.drawLine(p1, calcPoint);
					
					p.setPen(QPen(Qt::gray, 3.));
					p.setBrush(QColor(0,0,0,127));
					p.drawEllipse(calcPoint, 10, 10);
					
					p.restore();
		
					
					avgPoint2 += calcPoint;
					count ++;
				}
			}
			
			
			//p.setPen(QPen(Qt::gray, penWidth));
			//p.drawLine(p0, p1);
			
			//break;
		
		}
	}
	
// 	avgPoint2.setX( avgPoint.x() / count );
// 	avgPoint2.setY( avgPoint.y() / count );
	avgPoint2 /= count;

	
	{
		p.save();
		p.setPen(QPen(Qt::black, 5));
		p.setBrush(QColor(0,0,255,127));
		p.drawPolygon(userPoly);
		p.restore();
	}

	penWidth = 20;
	
// 	p.setPen(QPen(Qt::green, 10.));
// 	p.setBrush(QColor(0,0,0,127));
// 	if(!isnan(avgPoint.x()) && !isnan(avgPoint.y()))
// 		p.drawEllipse(avgPoint, penWidth, penWidth);
	
	if(!isnan(avgPoint2.x()) && !isnan(avgPoint2.y()))
	{
		if(avgPoint == avgPoint2)
			p.setPen(QPen(Qt::yellow, 10.));
		else
			p.setPen(QPen(Qt::red, 10.));
			
		p.setBrush(QColor(0,0,0,127));
		p.drawEllipse(avgPoint2, penWidth, penWidth);
		
		#ifdef OPENCV_ENABLED
		m_kalman.predictionUpdate((float)avgPoint2.x(), (float)avgPoint2.y());
		
		float x = avgPoint2.x(), y = avgPoint2.y();
		m_kalman.predictionReport(x, y);
		QPointF thisPredict(x,y);
		
		p.setPen(QPen(Qt::blue, 15.));
			
		p.setBrush(QColor(0,0,0,127));
		p.drawEllipse(thisPredict, penWidth, penWidth);
		#endif
	}
	
/*	
	p.setPen(QPen(Qt::blue,penWidth));
// 	QPointF s1 = line.p1() - itemWorldRect.topLeft();
// 	QPointF s2 = line.p2() - itemWorldRect.topLeft();
	QPointF s1 = line.p1();
	QPointF s2 = line.p2();
	
	//p.drawLine(s1,s2);
	
	QImage user(tr(":/data/images/%1/stock-media-rec.png").arg(size));
	p.drawImage((s1+s2)/2., user.scaled(128,128));
	
	p.setBrush(Qt::blue);
// 	p.drawRect(QRectF(s1.x()+3,s1.y()+3,6,6));
// 	p.drawRect(QRectF(s2.x()-3,s2.y()-3,6,6));
	
	p.setBrush(QBrush());
	//p.drawEllipse(center + QPointF(5.,5.), center.x(), center.y());
	p.setPen(QPen(Qt::white,penWidth));
// 	p.drawEllipse(p0, r0,r0);
	p.drawEllipse(p0, la,la);
	p.setPen(QPen(Qt::red,penWidth));
// 	p.drawEllipse(p1, r1,r1);
	p.drawEllipse(p1, lb,lb);
	p.setPen(QPen(Qt::green,penWidth));
	if(!p2.isNull())
		p.drawEllipse(p2, r2,r2);
	
	p.setPen(QPen(Qt::white,penWidth));
	p.drawRect(QRectF(p0-QPointF(5,5), QSizeF(10,10)));
	p.setPen(QPen(Qt::red,penWidth));
	p.drawRect(QRectF(p1-QPointF(5,5), QSizeF(10,10)));
	p.setPen(QPen(Qt::green,penWidth));
	if(!p2.isNull())
		p.drawRect(QRectF(p2-QPointF(5,5), QSizeF(10,10)));
	
// 	p.setPen(QPen(Qt::red,penWidth));
// 	p.drawLine(line);
// 	p.drawLine(line2);
// 	p.drawLine(line3);
	
	penWidth *=2;
	
	p.setPen(QPen(Qt::white,penWidth));
	p.drawLine(realLine);
	p.setPen(QPen(Qt::gray,penWidth));
	p.drawLine(realLine2);

	penWidth /=2;
	p.setPen(QPen(Qt::darkGreen,penWidth));
	p.drawLine(apLine);
	p.setPen(QPen(Qt::darkYellow,penWidth));
	p.drawLine(userLine);
	p.setPen(QPen(Qt::darkRed,penWidth));
	p.drawLine(userLine2);

// 	p.setPen(QPen(Qt::darkYellow,penWidth));
// 	p.drawRect(QRectF(calcPoint-QPointF(5,5), QSizeF(10,10)));
// 	p.setPen(QPen(Qt::darkGreen,penWidth));
// 	p.drawRect(QRectF(realPoint-QPointF(5,5), QSizeF(10,10)));
	
*/
	p.end();
	
	m_userItem->setPixmap(QPixmap::fromImage(image));
	//m_userItem->setOffset(-(((double)image.width())/2.), -(((double)image.height())/2.));
	//m_userItem->setPos(itemWorldRect.center());
	//m_userItem->setOffset(offset);
	m_userItem->setOffset(-origSize.width()/2, -origSize.height()/2);
	m_userItem->setPos(0,0);
	
// 	m_userItem->setOffset(0,0); //-(image.width()/2), -(image.height()/2));
// 	m_userItem->setPos(0,0); //itemWorldRect.center());
	
	m_userItem->setVisible(true);

	//m_mapWindow->setStatusMessage("Location Updated!", 100);

}

void MapGraphicsScene::updateApLocationOverlay()
{
	if(!m_autoGuessApLocations)
	{
		m_apLocationOverlay->setVisible(false);
		return;
	}
	
	if(m_sigValues.isEmpty())
		return;
	
	QSize origSize = m_bgPixmap.size();
	QImage image(QSize(origSize.width()*2,origSize.height()*2), QImage::Format_ARGB32_Premultiplied);
	QPointF offset = QPointF();
	
	memset(image.bits(), 0, image.byteCount());
	//image.fill(QColor(255,0,0,50).rgba());
	

	/// JUST for debugging
	// 	realPoint = m_sigValues.last()->point;
	// 	results   = m_sigValues.last()->scanResults;
	//

	if(m_sigValues.isEmpty())
	{
		qDebug() << "[apLocationGuess] No readings, can't guess anything";
	}


	QPainter p(&image);
	p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
	p.translate(origSize.width()/2,origSize.height()/2);


	#ifdef Q_OS_ANDROID
	QString size = "64x64";
	#else
	QString size = "32x32";
	#endif

	double penWidth = 5.0;

	QHash<QString,bool> drawnFlag;
	QHash<QString,int> badLossFactor;

	QVector<QPointF> userPoly;

	QPointF avgPoint(0.,0.);
	int count = 0;


	QHash<QString, QList<SigMapValue*> > valuesByAp;
	foreach(QString apMac, m_apInfo.keys())
	{
		// Build a list of values for this AP
		valuesByAp.insert(apMac, QList<SigMapValue *>());
		foreach(SigMapValue *val, m_sigValues)
			if(val->hasAp(apMac))
				valuesByAp[apMac].append(val);

		// Sort values by signal
		QList<SigMapValue *> list = valuesByAp[apMac];

		MapGraphicsScene_sort_apMac = apMac;
		qSort(list.begin(), list.end(), MapGraphicsScene_sort_SigMapValue_bySignal);

		valuesByAp[apMac] = list;


		MapApInfo *info = apInfo(apMac);
		
		qDebug() << "[apLocationGuess] Found "<<list.size()<<" readings for apMac:"<<apMac<<", ESSID:" << info->essid;

		if(list.isEmpty())
			continue;
		
		QColor color0 = baseColorForAp(apMac);

				
		QPointF avgPoint2(0.,0.);
		count = 0;
		int numVals = list.size();

		//SigMapValue *val0 = list.first();
		
		for(int i=0; i<numVals; i++)
		{
			//SigMapValue *val1 = list[i];
			SigMapValue *val0 = list[i];
			SigMapValue *val1 = (i < numVals - 1) ? list[i+1] : list[0];
		
			
			QPointF calcPoint = triangulateAp(apMac, val0, val1);

			// We assume triangulate() already stored drived loss factor into apInfo()
			//double r0 = dBmToDistance(val0->signalForAp(apMac, true), apMac) * m_pixelsPerMeter;
			//double r1 = dBmToDistance(val1->signalForAp(apMac, true), apMac) * m_pixelsPerMeter;

			qDebug() << "MapGraphicsScene::updateApLocationOverlay(): [apLocationGuess] ap:"<<apMac<<", reading"<<i<<": calcPoint:"<<calcPoint;

			if(isnan(calcPoint.x()) || isnan(calcPoint.y()))
				qDebug() << "MapGraphicsScene::updateApLocationOverlay(): [apLocationGuess] Can't render ellipse - calcPoint is NaN";
			else
			{
				//userPoly << calcPoint;

				p.setPen(QPen(color0, penWidth));//, Qt::DashLine));
				p.drawLine(val0->point, calcPoint);

				p.setPen(QPen(color0, penWidth));
				p.drawLine(val1->point, calcPoint);

				avgPoint2 += calcPoint;
				count ++;

				p.setPen(QPen(Qt::color0, 3.));
				p.setBrush(QColor(0,0,0,127));
				p.drawEllipse(val0->point, 10, 10);

				p.setPen(QPen(Qt::color0, 3.));
				p.setBrush(QColor(0,0,0,127));
				p.drawEllipse(val1->point, 10, 10);

				p.setPen(QPen(Qt::green, 3.));
				p.setBrush(QColor(0,0,0,127));
				p.drawEllipse(calcPoint, 10, 10);

			}

			//break;
		}

		avgPoint2 /= count;

		p.setPen(QPen(Qt::red, 30.));
		//p.drawPolygon(userPoly);

		penWidth = 5;

		p.setPen(QPen(Qt::green, 10.));
		p.setBrush(QColor(0,0,0,127));
		if(!isnan(avgPoint.x()) && !isnan(avgPoint.y()))
			p.drawEllipse(avgPoint, penWidth, penWidth);

		qDebug() << "MapGraphicsScene::updateApLocationOverlay(): [apLocationGuess] ap:"<<apMac<<", avgPoint2:"<<avgPoint2<<", count:"<<count;

		if(!isnan(avgPoint2.x()) && !isnan(avgPoint2.y()))
		{
			p.setPen(QPen(color0, 10.));
			p.setBrush(QColor(0,0,0,127));
			p.drawEllipse(avgPoint2, 64, 64);

			QImage markerGroup(":/data/images/ap-marker.png");
			p.drawImage(avgPoint2 - QPoint(markerGroup.width()/2,markerGroup.height()/2), markerGroup);
			//p.drawEllipse(avgPoint2, penWidth, penWidth);
				
// 				if(avgPoint == avgPoint2)
// 					p.setPen(QPen(Qt::yellow, 10.));
// 				else
					//p.setPen(QPen(Qt::green, 4)); //10.));


			qDrawTextO(p,(int)avgPoint2.x(),(int)avgPoint2.y(),apInfo(apMac)->essid);
			
			/*
			#ifdef OPENCV_ENABLED
			info->locationGuessKalman.predictionUpdate((float)avgPoint2.x(), (float)avgPoint2.y());

			float x = avgPoint2.x(), y = avgPoint2.y();
			info->locationGuessKalman.predictionReport(x, y);
			QPointF thisPredict(x,y);

			p.setPen(QPen(Qt::darkGreen, 15.));

			p.setBrush(QColor(0,0,0,127));
			p.drawEllipse(thisPredict, penWidth, penWidth);

			info->locationGuess = thisPredict;

			#else

			info->locationGuess = avgPoint2;
			
			#endif
			*/

			info->locationGuess = avgPoint2;
			if(!info->marked) // marked means user set point, so don't override if user marked
				info->point = avgPoint2;
		}

	}

	p.end();
	

	m_apLocationOverlay->setPixmap(QPixmap::fromImage(image));
	//m_userItem->setOffset(-(((double)image.width())/2.), -(((double)image.height())/2.));
	//m_userItem->setPos(itemWorldRect.center());
	//m_userItem->setOffset(offset);
	m_apLocationOverlay->setOffset(-origSize.width()/2, -origSize.height()/2);
	m_apLocationOverlay->setPos(0,0);
	
// 	m_userItem->setOffset(0,0); //-(image.width()/2), -(image.height()/2));
// 	m_userItem->setPos(0,0); //itemWorldRect.center());
	
	m_apLocationOverlay->setVisible(true);

}

QPointF MapGraphicsScene::triangulateAp(QString apMac, SigMapValue *val0, SigMapValue *val1)
{
	//MapApInfo *info = apInfo(apMac);

// 	if(info->lossFactor.isNull() ||
// 	  (info->lossFactorKey > -1 && info->lossFactorKey != m_sigValues.size()))
// 	{
// 		QPointF lossFactor0 = deriveImpliedLossFactor(ap0);
// 
// 		// Store newly-derived loss factors into apInfo() for use in dBmToDistance()
// 		info->lossFactor    = lossFactor0;
// 		info->lossFactorKey = m_sigValues.size();
// 	}


	// Get location of readings (in pixels, obviously)
	QPointF p0 = val0->point;
	QPointF p1 = val1->point;

	int dBm0 = (int)val0->signalForAp(apMac, true); // true = return dBm's not %
	int dBm1 = (int)val1->signalForAp(apMac, true); // ''

	// Get a line from one AP to the other (need this for length and angle)
	QLineF apLine(p1,p0);

	double lc = apLine.length();
	double la = dBmToDistance(dBm0, apMac) * m_pixelsPerMeter; // dBmToDistance returns meters, convert to pixels
	double lb = dBmToDistance(dBm1, apMac) * m_pixelsPerMeter;

//	qDebug() << "[dump0] "<<dBm0<<dBm1;
//	qDebug() << "[dump1] "<<la<<lb<<lc;
//	qDebug() << "[dump2] "<</*realAngle<<realAngle2<<*/apLine.angle();
//	qDebug() << "[dump3] "<<p0<<p1/*<<realPoint*/;
	//qDebug() << "[dump4] "<<realLine.angleTo(apLine)<<realLine2.angleTo(apLine)<<realLine2.angleTo(realLine);

	/*

	  C
	  *
	  |`.
	 b|  `. a
	  |    `.
	  |      `.
	A *--------* B
	      c

	*/

	// Calculate the angle (A)
	double cosA = (lb*lb + lc*lc - la*la)/(2*lb*lc);
	double angA = acos(cosA) * 180.0 / Pi;

	// Create a line from the appros AP and set it's angle to the calculated angle
	QLineF userLine(p1,QPointF());
	userLine.setAngle(angA + apLine.angle());
	userLine.setLength(lb);

	qDebug() << "MapGraphicsScene::triangulateAp("<<apMac<<","<<val0<<","<<val1<<"): ang(A):"<<angA<<", cos(A):"<<cosA;
	qDebug() << "MapGraphicsScene::triangulateAp("<<apMac<<","<<val0<<","<<val1<<"): apLine.angle:"<<apLine.angle();
	qDebug() << "MapGraphicsScene::triangulateAp("<<apMac<<","<<val0<<","<<val1<<"): la:"<<la<<", lb:"<<lb<<", lc:"<<lc;


	// Return the end-point of the line we just created - that's the point where
	// the signals of the two APs intersect (supposedly, any error in the result is largely from dBmToDistance(), and, by extension, deriveObservedLossFactor())
	return userLine.p2();
}

QPointF MapGraphicsScene::triangulate(QString ap0, int dBm0, QString ap1, int dBm1)
{
	MapApInfo *inf0 = apInfo(ap0);
	MapApInfo *inf1 = apInfo(ap1);
	
	if(inf0->lossFactor.isNull() ||
	  (inf0->lossFactorKey > -1 && inf0->lossFactorKey != m_sigValues.size()))
	{
		QPointF lossFactor0 = deriveObservedLossFactor(ap0);
		
		// Store newly-derived loss factors into apInfo() for use in dBmToDistance()
		inf0->lossFactor    = lossFactor0;
		inf0->lossFactorKey = m_sigValues.size();
	}
		
	if(inf1->lossFactor.isNull() ||
	  (inf1->lossFactorKey > -1 && inf1->lossFactorKey != m_sigValues.size()))
	{
		QPointF lossFactor1 = deriveObservedLossFactor(ap1);
		
		// Store newly-derived loss factors into apInfo() for use in dBmToDistance()
		inf1->lossFactor    = lossFactor1;
		inf1->lossFactorKey = m_sigValues.size();
	}
	
	// Get location of APs (in pixels, obviously)
	QPointF p0 = inf0->point;
	QPointF p1 = inf1->point;
	
	// Get a line from one AP to the other (need this for length and angle)
	QLineF apLine(p1,p0);
	
	double lc = apLine.length();
	double la = dBmToDistance(dBm0, ap0) * m_pixelsPerMeter; // dBmToDistance returns meters, convert to pixels
	double lb = dBmToDistance(dBm1, ap1) * m_pixelsPerMeter;
	
// 	qDebug() << "[dump1] "<<la<<lb<<lc;
// 	qDebug() << "[dump2] "<<realAngle<<realAngle2<<apLine.angle();
// 	qDebug() << "[dump3] "<<p0<<p1<<realPoint;
// 	qDebug() << "[dump4] "<<realLine.angleTo(apLine)<<realLine2.angleTo(apLine)<<realLine2.angleTo(realLine);
	
	/*
	
	  C
	  *
	  |`.
	 b|  `. a
	  |    `.
	  |      `.
	A *--------* B
	      c
	
	*/
	
	// Calculate the angle (A)
	double cosA = (lb*lb + lc*lc - la*la)/(2*lb*lc);
	double angA = acos(cosA) * 180.0 / Pi;
	
	// Create a line from the appros AP and set it's angle to the calculated angle
	QLineF userLine(p1,QPointF());
	userLine.setAngle(angA + apLine.angle());
	userLine.setLength(lb);
	
// 	qDebug() << "MapGraphicsScene::triangulate("<<ap0<<","<<dBm0<<","<<ap1<<","<<dBm1<<"): ang(A):"<<angA<<", cos(A):"<<cosA;
// 	qDebug() << "MapGraphicsScene::triangulate("<<ap0<<","<<dBm0<<","<<ap1<<","<<dBm1<<"): apLine.angle:"<<apLine.angle();
// 	qDebug() << "MapGraphicsScene::triangulate("<<ap0<<","<<dBm0<<","<<ap1<<","<<dBm1<<"): la:"<<la<<", lb:"<<lb<<", lc:"<<lc;
	
	
	// Return the end-point of the line we just created - that's the point where 
	// the signals of the two APs intersect (supposedly, any error in the result is largely from dBmToDistance(), and, by extension, deriveObservedLossFactor())
	return userLine.p2();
}

QLineF MapGraphicsScene::triangulate2(QString ap0, int dBm0, QString ap1, int dBm1)
{
	MapApInfo *inf0 = apInfo(ap0);
	MapApInfo *inf1 = apInfo(ap1);

	if(inf0->lossFactor.isNull() ||
	  (inf0->lossFactorKey > -1 && inf0->lossFactorKey != m_sigValues.size()))
	{
		QPointF lossFactor0 = deriveObservedLossFactor(ap0);

		// Store newly-derived loss factors into apInfo() for use in dBmToDistance()
		inf0->lossFactor    = lossFactor0;
		inf0->lossFactorKey = m_sigValues.size();
	}

	if(inf1->lossFactor.isNull() ||
	  (inf1->lossFactorKey > -1 && inf1->lossFactorKey != m_sigValues.size()))
	{
		QPointF lossFactor1 = deriveObservedLossFactor(ap1);

		// Store newly-derived loss factors into apInfo() for use in dBmToDistance()
		inf1->lossFactor    = lossFactor1;
		inf1->lossFactorKey = m_sigValues.size();
	}

	// Get location of APs (in pixels, obviously)
	QPointF p0 = inf0->point;
	QPointF p1 = inf1->point;

	// Get a line from one AP to the other (need this for length and angle)
	QLineF apLine(p1,p0);

	double lc = apLine.length();
	double la = dBmToDistance(dBm0, ap0) * m_pixelsPerMeter; // dBmToDistance returns meters, convert to pixels
	double lb = dBmToDistance(dBm1, ap1) * m_pixelsPerMeter;

// 	qDebug() << "[dump1] "<<la<<lb<<lc;
// 	qDebug() << "[dump2] "<<realAngle<<realAngle2<<apLine.angle();
// 	qDebug() << "[dump3] "<<p0<<p1<<realPoint;
// 	qDebug() << "[dump4] "<<realLine.angleTo(apLine)<<realLine2.angleTo(apLine)<<realLine2.angleTo(realLine);

	/*

	  C
	  *
	  |`.
	 b|  `. a
	  |    `.
	  |      `.
	A *--------* B
	      c

	*/

	// http://local.wasp.uwa.edu.au/~pbourke/geometry/2circle/:
/*	double ca   = (la*la - lb*lb + lc*lc) / (2*lc);
	double ch2  = la*la + ca*ca;
	double ch   = sqrt(ch2);
	QPointF p2  = QPointF(p0.x() + ca * (p1.x() - p0.x()) / lc,
			      p0.y() + ca * (p1.y() - p0.y()) / lc);
	QPointF p3a = QPointF(p2.x() + ch * (p1.y() - p0.y()) / lc,
			      p2.y() - ch * (p1.x() - p0.x()) / lc);
	QPointF p3b = QPointF(p2.x() - ch * (p1.y() - p0.y()) / lc,
			      p2.y() -+ch * (p1.x() - p0.x()) / lc);

	qDebug() << "triangulate2(): la:"<<la<<", lb:"<<lb<<", lc:"<<lc;
	qDebug() << "triangulate2(): ca:"<<ca<<", ch2:"<<ch2<<", ch:"<<ch;
	Debug() << "triangulate2(): p2:"<<p2<<", p3a:"<<p3a<<", p3b:"<<p3b;*/

	Q_UNUSED(lc);
	
	double xi, yi, xi_prime, yi_prime;
	int ret = circle_circle_intersection(
				p0.x(), p0.y(), la,
				p1.x(), p1.y(), lb,
				&xi, &yi,
				&xi_prime, &yi_prime);
	if(!ret)
	{
		qDebug() << "triangulate2(): circle_circle_intersection() returned 0";
		return QLineF();
	}

	return QLineF(xi, yi, xi_prime, yi_prime);
	//return QLineF(p3a, p3b);

	/*
	// Calculate the angle (A)
	double cosA = (lb*lb + lc*lc - la*la)/(2*lb*lc);
	double angA = acos(cosA) * 180.0 / Pi;

	// Create a line from the appros AP and set it's angle to the calculated angle
	QLineF userLine(p1,QPointF());
	userLine.setAngle(angA + apLine.angle());
	userLine.setLength(lb);

// 	qDebug() << "MapGraphicsScene::triangulate("<<ap0<<","<<dBm0<<","<<ap1<<","<<dBm1<<"): ang(A):"<<angA<<", cos(A):"<<cosA;
// 	qDebug() << "MapGraphicsScene::triangulate("<<ap0<<","<<dBm0<<","<<ap1<<","<<dBm1<<"): apLine.angle:"<<apLine.angle();
// 	qDebug() << "MapGraphicsScene::triangulate("<<ap0<<","<<dBm0<<","<<ap1<<","<<dBm1<<"): la:"<<la<<", lb:"<<lb<<", lc:"<<lc;


	// Return the end-point of the line we just created - that's the point where
	// the signals of the two APs intersect (supposedly, any error in the result is largely from dBmToDistance(), and, by extension, deriveObservedLossFactor())
	return userLine.p2();
	*/
}

double MapGraphicsScene::deriveLossFactor(QString apMac, int pRx, double distMeters, double gRx)
{
	const double m   =  0.12; // (meters) - wavelength of 2442 MHz, freq of 802.11b/g radio
	const double Xa  =  3.00; // (double)rand()/(double)RAND_MAX * 17. + 3.; // normal rand var, std dev a=[3,20]
	
	MapApInfo *info = apInfo(apMac);
	double pTx = info->txPower;
	double gTx = info->txGain;
	
	double logDist       = log10(distMeters);
	double adjustedPower = (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi));
	double n             = 1./(logDist / adjustedPower)/10.;
	
	return n;
}

QPointF MapGraphicsScene::deriveObservedLossFactor(QString apMac)
{
	const double m   =  0.12; // (meters) - wavelength of 2442 MHz, freq of 802.11b/g radio
	const double Xa  =  3.00; // (double)rand()/(double)RAND_MAX * 17. + 3.; // normal rand var, std dev a=[3,20]
	
	MapApInfo *info = apInfo(apMac);
	double pTx = info->txPower;
	double gTx = info->txGain;
	
	int shortCutoff = info->shortCutoff;
	
	QPointF lossFactor(2.,2.);
	
	int method = 1;
	if(method == 1)
	{
		// Use a reworked distance formula to calculate lossFactor for each point, then average together
			
		double shortFactor = 0.,
		       longFactor  = 0.;
		
		int    shortCount  = 0,
		       longCount   = 0;
		
		foreach(SigMapValue *val, m_sigValues)
		{
			if(val->hasAp(apMac))
			{
				double pRx = val->signalForAp(apMac, true); // true = return raw dBM
				double gRx = val->rxGain;
				
				// Assuming: logDist = (1/(10*n)) * (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi));
				// Rearranging gives:
				// logDist = (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi)) / (10n)
				// =
				// 1/10n = logDist / (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi))
				// =
				// n = 1/(logDist / (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi)))/10
				//
				// Paraphrased: 
				// adjustedPower = (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi))
				// logDist = 1/10n * adjustedPower 
				// 	(or simply: logDist = adjustedPower/(10n))
				// n = 1/(logDist/adjustedPower)/10
				// Where logDist = log(distFromAp * metersPerPixel)
				// NOTE: Any log above is base 10 as in log10(), above, not natrual logs as in log()
				
				double logDist       = log10(QLineF(info->point,val->point).length() / m_pixelsPerMeter);
				double adjustedPower = (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi));
				double n             = 1./(logDist / adjustedPower)/10.;
				
				if(pRx > shortCutoff)
				{
					shortFactor += n;
					shortCount  ++;
				}
				else
				{
					longFactor  += n;
					longCount   ++;
				}
			}
		}
		
		if(!shortCount)
		{
			shortFactor = 2.;
			shortCount  = 1;
		}
		
		if(!longCount)
		{
			longFactor = 1.;
			longCount  = 1;
		}
		
		lossFactor = QPointF( longFactor /  longCount,
				     shortFactor / shortCount);
	
		// TODO calc MSE for the resultant N as error += ( (dBmToDistance(dBm, lossFactor)-realDist)^2 ) foreach reading; error/= readings
	
	}
	else
	if(method == 2)
	{
		// Try to minimize MSE using a gradient decent search for n
		// Stop when MSE < 0.1 OR num iterations > 1_000_000 OR zig-zag
		
		const double errorCutoff = 0.01; // minimum eror
		const int maxIterations = 1000000;
		
		double error = 1.0; // starting error, meaningless
		int numIterations = 0;
		
		double shortFactor = 6.0; // starting factor guesses
		double longFactor  = 2.0; // starting factor guesses
		
		double stepFactor = 0.001; // minimum step 
		double stepChangeFactor = 0.0001; // amount to change step by 
		double shortStep = -0.5; // start with large steps, decrease as we get closer
		double longStep  = -0.5; // start with large steps, decrease as we get closer
		
		int shortCutoff = info->shortCutoff; // typically -49 dBm, ghreater than this, pRx is assumed less than 15 meters to AP, less than -49, assumed further than 15m from AP
		
		// Values to prevent zig-zagging around local minima
		double lastError = 0.0;
		double lastError1 = 0.0;
		bool zigZag = false;
		
		while(error > errorCutoff && numIterations < maxIterations && !zigZag)
		{
			double shortErrorSum = 0.;
			int shortErrorCount  = 0;
			
			double longErrorSum  = 0.;
			int longErrorCount   = 0;
			
			foreach(SigMapValue *val, m_sigValues)
			{
				if(val->hasAp(apMac))
				{
					double pRx = val->signalForAp(apMac, true); // true = return raw dBM
					double gRx = val->rxGain;
					double realDist = QLineF(info->point,val->point).length() / m_pixelsPerMeter;
				
					double n = pRx < shortCutoff ? longFactor : shortFactor;
					
					double logDist = (1/(10*n)) * (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi));
					double invLog  = pow(10, logDist); // distance in meters
					
					double err = realDist - invLog; //pow(invLog - realDist, 2);
					
					if(pRx < shortCutoff)
					{
						longErrorSum   += err;
						longErrorCount ++;
					}
					else
					{
						shortErrorSum   += err;
						shortErrorCount ++;
					}
				}
			}
			
			double shortErrAvg = shortErrorSum / shortErrorCount;
			double longErrAvg  =  longErrorSum /  longErrorCount;
			error = (fabs(shortErrAvg) + fabs(longErrAvg)) / 2; // used for termination
			
			if(lastError1 == error)
				zigZag = true;
			
			lastError1 = lastError;
			lastError = error;
			
			shortStep = ((shortErrAvg > 0) ? -1 : 1) * fabs(shortStep) * stepChangeFactor;
			longStep  = ((longErrAvg  > 0) ? -1 : 1) * fabs(longStep)  * stepChangeFactor;
			
			int ss = shortStep < 0 ? -1:1;
			int ls = longStep  < 0 ? -1:1;
			shortStep = qMax(stepFactor, fabs(shortStep)) * ss;
			longStep  = qMax(stepFactor, fabs(longStep))  * ls;
			
			shortFactor += shortStep;
			longFactor  += longStep;
			
			qDebug() << "MapGraphicsScene::deriveObservedLossFactor(): "<<numIterations<<":"<<error<<" (z:"<<zigZag<<"): shortErr:"<<shortErrAvg<<", longErr:"<<longErrAvg<<", shortFactor:"<<shortFactor<<", longFactor:"<<longFactor<<", shortStep:"<<shortStep<<", longStep:"<<longStep;
			
			numIterations ++;
		}

		lossFactor = QPointF( longFactor, shortFactor );
	}

	//qDebug() << "MapGraphicsScene::deriveObservedLossFactor(): "<<apMac<<": Derived: "<<lossFactor;
	//qDebug() << "MapGraphicsScene::deriveObservedLossFactor(): "<<apMac<<": Debug: pTx:"<<pTx<<", gTx:"<<gTx<<" shortCutoff:"<<shortCutoff;
	
	return lossFactor;
}

// TODO 'implied' loss factor should be a loss factor we can use to triangulate the location of the *AP*.
// So we need to derive a loss factor implied by the signal variance between readings at different (known) locations, given the distance between those locations.
// The code below DOES NOT do any of that - it's just here as a placeholder till I get around to reworking it
QPointF MapGraphicsScene::deriveImpliedLossFactor(QString apMac)
{
	const double m   =  0.12; // (meters) - wavelength of 2442 MHz, freq of 802.11b/g radio
	const double Xa  =  3.00; // (double)rand()/(double)RAND_MAX * 17. + 3.; // normal rand var, std dev a=[3,20]

	MapApInfo *info = apInfo(apMac);
	double pTx = info->txPower;
	double gTx = info->txGain;

	int shortCutoff = info->shortCutoff;

	QPointF lossFactor(2.,2.);

	// Use a reworked distance formula to calculate lossFactor for each point, then average together

	double shortFactor = 0.,
		longFactor  = 0.;

	int    shortCount  = 0,
		longCount   = 0;

	foreach(SigMapValue *val, m_sigValues)
	{
		if(val->hasAp(apMac))
		{
			double pRx = val->signalForAp(apMac, true); // true = return raw dBM
			double gRx = val->rxGain;

			// TODO Rework formula and this algorithm to account for the distance between two points instead of point and AP

			// Assuming: logDist = (1/(10*n)) * (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi));
			// Rearranging gives:
			// logDist = (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi)) / (10n)
			// =
			// 1/10n = logDist / (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi))
			// =
			// n = 1/(logDist / (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi)))/10
			//
			// Paraphrased:
			// adjustedPower = (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi))
			// logDist = 1/10n * adjustedPower
			// 	(or simply: logDist = adjustedPower/(10n))
			// n = 1/(logDist/adjustedPower)/10
			// Where logDist = log(distFromAp * metersPerPixel)
			// NOTE: Any log above is base 10 as in log10(), above, not natrual logs as in log()

			double logDist       = log10(QLineF(info->point,val->point).length() / m_pixelsPerMeter);
			double adjustedPower = (pTx - pRx + gTx + gRx - Xa + 20*log10(m) - 20*log10(4*Pi));
			double n             = 1. / (logDist / adjustedPower) / 10.;

			if(pRx > shortCutoff)
			{
				shortFactor += n;
				shortCount  ++;
			}
			else
			{
				longFactor  += n;
				longCount   ++;
			}
		}
	}

	if(!shortCount)
	{
		shortFactor = 2.;
		shortCount  = 1;
	}

	if(!longCount)
	{
		longFactor = 1.;
		longCount  = 1;
	}

	lossFactor = QPointF( longFactor /  longCount,
				shortFactor / shortCount);

	// TODO calc MSE for the resultant N as error += ( (dBmToDistance(dBm, lossFactor)-realDist)^2 ) foreach reading; error/= readings
	return lossFactor;
}

		
double MapGraphicsScene::dBmToDistance(int dBm, QString apMac, double gRx)
{
	MapApInfo *info = apInfo(apMac);
	QPointF n  = info->lossFactor;
	int cutoff = info->shortCutoff;
	double pTx = info->txPower;
	double gTx = info->txGain;
	return dBmToDistance(dBm, n, cutoff, pTx, gTx, gRx);
}

double MapGraphicsScene::dBmToDistance(int dBm, QPointF lossFactor, int shortCutoff, double txPower, double txGain,  double rxGain)
{
// 	double n   = /* 1.60*/0; /// TUNE THIS 
// 	n = apMacToDbm[ap1] < -49 ? 1.6 : 2.0;
// 	double m   =  0.12; // (meters) - wavelength of 2442 MHz, freq of 802.11b/g radio
// 	double Xa  =  3.00; // (double)rand()/(double)RAND_MAX * 17. + 3.; // normal rand var, std dev a=[3,20]
// 	double pTx = 11.80; // (dBm) - Transmit power, [1] est stock WRT54GL at 19 mW, [1]=http://forums.speedguide.net/showthread.php?253953-Tomato-and-Linksys-WRT54GL-transmit-power
// 	double pRx = apMacToDbm[ap1]; /// Our measurement
// 	double gTx =  5.00; // (dBi) - Gain of stock WRT54GL antennas
// 	double gRx =  3.00; // (dBi) - Estimated gain of laptop antenna, neg dBi. Internal atennas for phones/laptops range from (-3,+3)
	
	// NOTE Formula used below is from from http://web.mysites.ntu.edu.sg/aschfoh/public/Shared%20Documents/pub/04449717-icics07.pdf
	// double logDist = (1/(10*n)) * (pTx - pRx + gTx + gRx - Xa + 20*log(m) - 20*log(4*Pi));
	
	// Possibly integrate method of solving loss factor as described in http://continuouswave.com/whaler/reference/pathLoss.html 
	
	// Alternative forumla given at http://www.moxa.com/newsletter/connection/2008/03/Figure_out_transmission_distance_from_wireless_device_specs.htm#04
	// Merging the two provided gives:
	// r=pow(10,(pTx+(gTx+gRx)*n-pRx)/20) / (41.88 * f) where r is in km and f is the frequency, e.g. 2442
	
	
	// n is the loss factor describing obstacles, walls, reflection, etc, can be derived from existing data with driveObservedLossFactor()
	double n   = dBm < shortCutoff ? lossFactor.x() : lossFactor.y();
	
	double m   =  0.12; // (meters) - wavelength of 2442 MHz, freq of 802.11b/g radio
	double Xa  =  3.00; // (double)rand()/(double)RAND_MAX * 17. + 3.; // normal rand var, std dev a=[3,20]
	
	double logDist = (1/(10*n)) * (txPower - dBm + txGain + rxGain - Xa + 20*log10(m) - 20*log10(4*Pi));
	double distMeters = pow(10, logDist); // distance in meters
	
	//qDebug() << "MapGraphicsScene::dBmToDistance(): "<<dBm<<": meters:"<<distMeters<<", Debug: n:"<<n<<", txPower:"<<txPower<<", txGain:"<<txGain<<", rxGain:"<<rxGain<<", logDist:"<<logDist;
	
	return distMeters;
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
		
		//m_mapWindow->setStatusMessage("");
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

/// ImageFilters class, inlined here for ease of implementation

class ImageFilters
{
public:
	static QImage blurred(const QImage& image, const QRect& rect, int radius);
	
	// Modifies 'image'
	static void blurImage(QImage& image, int radius, bool highQuality = false);
	static QRectF blurredBoundingRectFor(const QRectF &rect, int radius);
	static QSizeF blurredSizeFor(const QSizeF &size, int radius);

};

// Qt 4.6.2 includes a wonderfully optimized blur function in /src/gui/image/qpixmapfilter.cpp
// I'll just hook into their implementation here, instead of reinventing the wheel.
extern void qt_blurImage(QImage &blurImage, qreal radius, bool quality, int transposed = 0);

const qreal radiusScale = qreal(1.5);

// Copied from QPixmapBlurFilter::boundingRectFor(const QRectF &rect)
QRectF ImageFilters::blurredBoundingRectFor(const QRectF &rect, int radius) 
{
	const qreal delta = radiusScale * radius + 1;
	return rect.adjusted(-delta, -delta, delta, delta);
}

QSizeF ImageFilters::blurredSizeFor(const QSizeF &size, int radius)
{
	const qreal delta = radiusScale * radius + 1;
	QSizeF newSize(size.width()  + delta, 
	               size.height() + delta);
	
	return newSize;
}

// Modifies the input image, no copying
void ImageFilters::blurImage(QImage& image, int radius, bool highQuality)
{
	qt_blurImage(image, radius, highQuality);
}

// Blur the image according to the blur radius
// Based on exponential blur algorithm by Jani Huhtanen
// (maximum radius is set to 16)
QImage ImageFilters::blurred(const QImage& image, const QRect& /*rect*/, int radius)
{
	QImage copy = image.copy();
	qt_blurImage(copy, radius, false);
	return copy;
}

/// End ImageFilters implementation

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
	
// 	if(resultCounter == 1)
// 		boundingRect = iconRects[0] = QRectF(-iconSizeHalf, -iconSizeHalf, (double)iconSize, (double)iconSize);
	
	boundingRect.adjust(-10,-10,+10,+10);
	
	//qDebug() << "MapGraphicsScene::addSignalMarker(): boundingRect:"<<boundingRect;
	
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
	
	markerGroup = addDropShadow(markerGroup, (double)iconSize / 2.);
	
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
	item->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
	item->setZValue(99);
	
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

QImage MapGraphicsScene::addDropShadow(QImage markerGroup, double shadowSize)
{
	// Add in drop shadow
	if(shadowSize > 0.0)
	{
// 		double shadowOffsetX = 0.0;
// 		double shadowOffsetY = 0.0;
		QColor shadowColor = Qt::black;
		
		// create temporary pixmap to hold a copy of the text
		QSizeF blurSize = ImageFilters::blurredSizeFor(markerGroup.size(), (int)shadowSize);
		//qDebug() << "Blur size:"<<blurSize<<", doc:"<<doc.size()<<", shadowSize:"<<shadowSize;
		QImage tmpImage(blurSize.toSize(),QImage::Format_ARGB32_Premultiplied);
		memset(tmpImage.bits(),0,tmpImage.byteCount()); // init transparent
		
		// render the text
		QPainter tmpPainter(&tmpImage);
		
		tmpPainter.save();
		tmpPainter.translate(shadowSize, shadowSize);
		tmpPainter.drawImage(0,0,markerGroup);
		tmpPainter.restore();
		
		// blacken the image by applying a color to the copy using a QPainter::CompositionMode_DestinationIn operation. 
		// This produces a homogeneously-colored pixmap.
		QRect rect = tmpImage.rect();
		tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		tmpPainter.fillRect(rect, shadowColor);
		tmpPainter.end();
	
		// blur the colored image
		ImageFilters::blurImage(tmpImage, (int)shadowSize);
		
		// Render the original image back over the shadow
		tmpPainter.begin(&tmpImage);
		tmpPainter.save();
// 		tmpPainter.translate(shadowOffsetX - shadowSize,
// 				     shadowOffsetY - shadowSize);
		tmpPainter.translate(shadowSize, shadowSize);
		tmpPainter.drawImage(0,0,markerGroup);
		tmpPainter.restore();
		
		markerGroup = tmpImage;
	}
	
	return markerGroup;
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
		QColor baseColor = apMac.isEmpty() || apMac == "(default)" ? Qt::white : baseColorForAp(apMac);
		
		qDebug() << "MapGraphicsScene::colorForSignal(): Gradient cache miss for: "<<apMac;
		// paint the gradient
		#ifdef Q_OS_ANDROID
		int imgHeight = 1;
		#else
		int imgHeight = 10; // for debugging output
		#endif
		
		QImage signalLevelImage(2500,imgHeight,QImage::Format_ARGB32_Premultiplied);
		QPainter sigPainter(&signalLevelImage);
		
		if(1/*m_renderMode == RenderCircles ||
		   m_renderMode == RenderTriangles*/)
		{
			QLinearGradient fade(QPoint(0,0),QPoint(signalLevelImage.width(),0));
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
			//sigPainter.setCompositionMode(QPainter::CompositionMode_HardLight);
			//sigPainter.fillRect( signalLevelImage.rect(), baseColor );
		
		}
		else
		{
			//sigPainter.fillRect( signalLevelImage.rect(), baseColor ); 
		
			QLinearGradient fade(QPoint(0,0),QPoint(signalLevelImage.width(),0));
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
		for(int i=0; i<2500; i++)
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
	
	int listSize = colorList.size();
	int colorIdx = (int)(sig * (double)listSize);
	if(colorIdx > listSize-1)
		colorIdx = listSize-1;
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
	
	m_colorCounter = 0; // reset AP color counter
	
	QSettings data(filename, QSettings::IniFormat);
	qDebug() << "MapGraphicsScene::loadResults(): Loading from: "<<filename;
	
	QFileInfo info(filename);
	if(info.exists() && !info.isWritable())
	{
		QMessageBox::warning(0, "Warning", tr("%1 is READ ONLY - you won't be able to save any changes!").arg(filename));
	}
	
	
	clear(); // clear and reset the map
	
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
	showReadingMarkers 	= settings.value("ropts-showReadingMarkers", 	true).toBool();
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

