#include "MapGraphicsScene.h"
#include "MapWindow.h"

#define VERBOSE_USER_GRAPHICS

QColor darkenColor(QColor color, double value)
{
	int h = color.hue();
	int s = color.saturation();
	int v = color.value();
	return QColor::fromHsv(h, s, (int)(v * value));
}

/// \brief Calculates the weight of point \a o at point \a i given power \a p
#define _dist2(a, b) ( ( ( a.x() - b.x() ) * ( a.x() - b.x() ) ) + ( ( a.y() - b.y() ) * ( a.y() - b.y() ) ) )

inline double inverseDistanceWeight(QPointF i, QPointF o, double p)
{
	double b = pow(sqrt(_dist2(i,o)),p);
	return b == 0 ? 0 : 1 / b;
}

/// These location routines are put here separate from MapGraphicsScene.cpp for easier editing and shorter compilation routines.
/// In the future, I probably should look at separating these into their own separate class.

static QString MapGraphicsScene_sort_apMac;

bool MapGraphicsScene_sort_SigMapValue_bySignal(SigMapValue *a, SigMapValue *b)
{
	QString apMac  = MapGraphicsScene_sort_apMac;
	if(!a || !b) return false;

	double va = a && a->hasAp(apMac) ? a->signalForAp(apMac) : 0.;
	double vb = b && b->hasAp(apMac) ? b->signalForAp(apMac) : 0.;
	return vb < va;

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
  double a, dx, dy, d, h, rx, ry, hx;
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
  hx = (r0*r0) - (a*a);
  if(hx < 0)
	  hx = 0;
  h = sqrt(hx);

  /* Now determine the offsets of the intersection points from
   * point 2.
   */
  rx = -dy * (h/d);
  ry = dx * (h/d);

  //qDebug() << "circle_circle_intersection: dx:"<<dx<<", dy:"<<dy<<", d:"<<d<<", a:"<<a<<", x2:"<<x2<<", y2:"<<y2<<", h:"<<h<<", rx:"<<rx<<", ry:"<<ry<<", r0:"<<r0<<", hx:"<<hx;

  /* Determine the absolute intersection points. */
  *xi = x2 + rx;
  *xi_prime = x2 - rx;
  *yi = y2 + ry;
  *yi_prime = y2 - ry;

  return 1;
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

#if 0

#include <stdio.h>
#include <math.h>

/* From http://en.wikipedia.org/wiki/Talk%3ATrilateration#Example_C_program */

/* No rights reserved (CC0, see http://wiki.creativecommons.org/CC0_FAQ).
 * The author has waived all copyright and related or neighboring rights
 * to this program, to the fullest extent possible under law.
*/

/* Largest nonnegative number still considered zero */
#define   MAXZERO  0.0

typedef struct vec3d    vec3d;
struct vec3d {
        double  x;
        double  y;
        double  z;
};

/* Return the difference of two vectors, (vector1 - vector2). */
vec3d vdiff(const vec3d vector1, const vec3d vector2)
{
        vec3d v;
        v.x = vector1.x - vector2.x;
        v.y = vector1.y - vector2.y;
        v.z = vector1.z - vector2.z;
        return v;
}

/* Return the sum of two vectors. */
vec3d vsum(const vec3d vector1, const vec3d vector2)
{
        vec3d v;
        v.x = vector1.x + vector2.x;
        v.y = vector1.y + vector2.y;
        v.z = vector1.z + vector2.z;
        return v;
}

/* Multiply vector by a number. */
vec3d vmul(const vec3d vector, const double n)
{
        vec3d v;
        v.x = vector.x * n;
        v.y = vector.y * n;
        v.z = vector.z * n;
        return v;
}

/* Divide vector by a number. */
vec3d vdiv(const vec3d vector, const double n)
{
        vec3d v;
        v.x = vector.x / n;
        v.y = vector.y / n;
        v.z = vector.z / n;
        return v;
}

/* Return the Euclidean norm. */
double vnorm(const vec3d vector)
{
        return sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
}

/* Return the dot product of two vectors. */
double dot(const vec3d vector1, const vec3d vector2)
{
        return vector1.x * vector2.x + vector1.y * vector2.y + vector1.z * vector2.z;
}

/* Replace vector with its cross product with another vector. */
vec3d cross(const vec3d vector1, const vec3d vector2)
{
        vec3d v;
        v.x = vector1.y * vector2.z - vector1.z * vector2.y;
        v.y = vector1.z * vector2.x - vector1.x * vector2.z;
        v.z = vector1.x * vector2.y - vector1.y * vector2.x;
        return v;
}

/* Return zero if successful, negative error otherwise.
 * The last parameter is the largest nonnegative number considered zero;
 * it is somewhat analoguous to machine epsilon (but inclusive).
*/
int trilateration(vec3d *const result1, vec3d *const result2,
                  const vec3d p1, const double r1,
                  const vec3d p2, const double r2,
                  const vec3d p3, const double r3,
                  const double maxzero)
{
        vec3d   ex, ey, ez, t1, t2;
        double  h, i, j, x, y, z, t;

        /* h = |p2 - p1|, ex = (p2 - p1) / |p2 - p1| */
        ex = vdiff(p2, p1);
        h = vnorm(ex);
        if (h <= maxzero) {
                /* p1 and p2 are concentric. */
                return -1;
        }
        ex = vdiv(ex, h);

        /* t1 = p3 - p1, t2 = ex (ex . (p3 - p1)) */
        t1 = vdiff(p3, p1);
        i = dot(ex, t1);
        t2 = vmul(ex, i);

        /* ey = (t1 - t2), t = |t1 - t2| */
        ey = vdiff(t1, t2);
        t = vnorm(ey);
        if (t > maxzero) {
                /* ey = (t1 - t2) / |t1 - t2| */
                ey = vdiv(ey, t);

                /* j = ey . (p3 - p1) */
                j = dot(ey, t1);
        } else
                j = 0.0;

        /* Note: t <= maxzero implies j = 0.0. */
        if (fabs(j) <= maxzero) {
                /* p1, p2 and p3 are colinear. */

                /* Is point p1 + (r1 along the axis) the intersection? */
                t2 = vsum(p1, vmul(ex, r1));
                if (fabs(vnorm(vdiff(p2, t2)) - r2) <= maxzero &&
                    fabs(vnorm(vdiff(p3, t2)) - r3) <= maxzero) {
                        /* Yes, t2 is the only intersection point. */
                        if (result1)
                                *result1 = t2;
                        if (result2)
                                *result2 = t2;
                        return 0;
                }

                /* Is point p1 - (r1 along the axis) the intersection? */
                t2 = vsum(p1, vmul(ex, -r1));
                if (fabs(vnorm(vdiff(p2, t2)) - r2) <= maxzero &&
                    fabs(vnorm(vdiff(p3, t2)) - r3) <= maxzero) {
                        /* Yes, t2 is the only intersection point. */
                        if (result1)
                                *result1 = t2;
                        if (result2)
                                *result2 = t2;
                        return 0;
                }

                return -2;
        }

        /* ez = ex x ey */
        ez = cross(ex, ey);

        x = (r1*r1 - r2*r2) / (2*h) + h / 2;
        y = (r1*r1 - r3*r3 + i*i) / (2*j) + j / 2 - x * i / j;
        z = r1*r1 - x*x - y*y;
        if (z < -maxzero) {
                /* The solution is invalid. */
                return -3;
        } else
        if (z > 0.0)
                z = sqrt(z);
        else
                z = 0.0;

        /* t2 = p1 + x ex + y ey */
        t2 = vsum(p1, vmul(ex, x));
        t2 = vsum(t2, vmul(ey, y));

        /* result1 = p1 + x ex + y ey + z ez */
        if (result1)
                *result1 = vsum(t2, vmul(ez, z));

        /* result1 = p1 + x ex + y ey - z ez */
        if (result2)
                *result2 = vsum(t2, vmul(ez, -z));

        return 0;
}
#endif


// double operator-(const QPointF &a, const QPointF &b)
// {
// 	return sqrt(dist2(a,b));
// }

QPointF operator*(const QPointF&a, const QPointF& b) { return QPointF(a.x()*b.x(), a.y()*b.y()); }

void MapGraphicsScene::updateDrivedLossFactor(MapApInfo *info)
{
	if(info->lossFactor.isNull() ||
	  (info->lossFactorKey > -1 && info->lossFactorKey != m_sigValues.size()))
	{
		QPointF lossFactor = deriveObservedLossFactor(info->mac);

		// Store newly-derived loss factors into apInfo() for use in dBmToDistance()
		info->lossFactor    = lossFactor;
		info->lossFactorKey = m_sigValues.size();
	}
}

QImage MapGraphicsScene::updateUserLocationOverlay(double rxGain, bool renderImage, QPointF truthTestPoint, QImage paintOnMe)
{
	if(!m_showMyLocation && renderImage)
	{
		//m_userItem->setVisible(false);
		return QImage();
	}

	// We still can estimate location without any signal readings - we'll just use default loss factors
	/*
	if(m_sigValues.isEmpty())
		return;
	*/

	//return;


	QSize origSize = m_bgPixmap.size();
	if(origSize.width() * 2 * origSize.height() * 2 * 3 > 256*1024*1024)
	{
		qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): Size too large, not updating: "<<origSize;
		m_userItem->setVisible(false);
		return QImage();
	}

	QImage image;
	
	if(renderImage)
	{
		if(!paintOnMe.isNull())
			image = paintOnMe;
		else
		{
		#ifdef Q_OS_ANDROID
			image = QImage(QSize(origSize.width()*1,origSize.height()*1), QImage::Format_ARGB32_Premultiplied);
		#else
			image = QImage(QSize(origSize.width()*2,origSize.height()*2), QImage::Format_ARGB32_Premultiplied);
		#endif
			memset(image.bits(), 0, image.byteCount());
			//image.fill(QColor(255,0,0,50).rgba());
		}
	}
	
	QPointF offset = QPointF();



	/// JUST for debugging
	// 	realPoint = m_sigValues.last()->point;
	// 	results   = m_sigValues.last()->scanResults;
	//

	QPointF userLocation  = QPointF(-1000,-1000);

	// Need at least two *known* APs registered on the map - with out that, we don't even need to check the results
	if(m_apInfo.values().size() < 2)
	{
		qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): Less than two APs observed, unable to guess user location";
		m_userItem->setVisible(false);
		return QImage();
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
		//m_mapWindow->setStatusMessage("Need at least 2 APs visible to calculate location", 2000);
		m_userItem->setVisible(false);
		return QImage();
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
	QPainter *p = 0;
	
	if(renderImage)
	{
		p = new QPainter(&image);
		p->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
	
	#ifndef Q_OS_ANDROID
		// on Android, we don't use a "x2" size, so translate is not needed
		p->translate(origSize.width()/2,origSize.height()/2);
	#endif
	}

	//QPainter p(&image);

	//p->setPen(Qt::black);

//	QRectF rect(QPointF(0,0),itemWorldRect.size());
// 	QRectF rect = itemWorldRect;
// 	QPointF center = rect.center();

	#ifdef Q_OS_ANDROID
	QString size = "64x64";
	#else
	QString size = "32x32";
	#endif

	double penWidth = 1.0 * m_pixelsPerMeter;

	QHash<QString,bool> drawnFlag;
	QHash<QString,int> badLossFactor;

	QVector<QPointF> userPoly;

	int count = 0;

	int numAps = apsVisible.size();


	QPointF avgPoint(0.,0.);
	count = 0;

	// The "best guess" at the "correct" intersection of the circles
	QList<QPointF> goodPoints;
	QLineF firstSet;

	QStringList pairsTested;

	if(renderImage)
	{
		QFont font = p->font();
		font.setPointSizeF(6 * m_pixelsPerFoot);
		p->setFont(font);
	}

	QHash<QString,double> distOverride;


#if 0
	if(numAps >= 3)
	{
		QString ap0 = apsVisible[0];
		QString ap1 = apsVisible[1];
		QString ap2 = apsVisible[2];

		MapApInfo *info0 = apInfo(ap0);
		MapApInfo *info1 = apInfo(ap1);
		MapApInfo *info2 = apInfo(ap2);

		QPointF p0 = info0->point;
		QPointF p1 = info1->point;
		QPointF p2 = info2->point;

		QColor color0 = baseColorForAp(ap0);
		QColor color1 = baseColorForAp(ap1);
		QColor color2 = baseColorForAp(ap2);

		// We assume triangulate() already stored drived loss factor into apInfo()
		double r0 = dBmToDistance(apMacToDbm[ap0], ap0) * m_pixelsPerMeter;
		double r1 = dBmToDistance(apMacToDbm[ap1], ap1) * m_pixelsPerMeter;
		double r2 = dBmToDistance(apMacToDbm[ap2], ap2) * m_pixelsPerMeter;

		#ifdef VERBOSE_USER_GRAPHICS
		// Render the estimated circles covered by these APs
		if(renderImage)
		{
			if(!drawnFlag.contains(ap0))
			{
				drawnFlag.insert(ap0, true);
	
				p->setPen(QPen(color0, penWidth));
	
				if(isnan(r0))
					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<"->"<<ap1<<"->"<<ap2<<": - Can't render ellipse p0/r0 - radius 0 is NaN";
				else
					p->drawEllipse(p0, r0, r0);
			}
	
			if(!drawnFlag.contains(ap1))
			{
				drawnFlag.insert(ap1, true);
	
				p->setPen(QPen(color1, penWidth));
	
				if(isnan(r1))
					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<"->"<<ap1<<"->"<<ap2<<": - Can't render ellipse p1/r1 - radius 1 is NaN";
				else
					p->drawEllipse(p1, r1, r1);
	
			}
	
			if(!drawnFlag.contains(ap2))
			{
				drawnFlag.insert(ap2, true);
	
				p->setPen(QPen(color2, penWidth));
	
				if(isnan(r2))
					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<"->"<<ap1<<"->"<<ap2<<": - Can't render ellipse p2/r2 - radius 2 is NaN";
				else
					p->drawEllipse(p2, r2, r2);
	
			}
		}
		#endif

		vec3d   v0, v1, v2, o1, o2;
		int     result;

// 		&p1.x, &p1.y, &p1.z, &r1,
// 		&p2.x, &p2.y, &p2.z, &r2,
// 		&p3.x, &p3.y, &p3.z, &r3) == 12) {

		v0.x = p0.x();
		v0.y = p0.y();
		v0.z = 0.0;

		v1.x = p1.x();
		v1.y = p1.y();
		v1.z = 0.0;

		v2.x = p2.x();
		v2.y = p2.y();
		v2.z = 0.0;
		
		result = trilateration(&o1, &o2, v0, r0, v1, r1, v2, r2, MAXZERO);

		QPointF goodPoint;

		if (result)
			printf("[user locate] No solution (%d).\n", result);
		else {
			printf("[user locate] Solutions:\n");
			printf("Solution 1: %g %g %g\n", o1.x, o1.y, o1.z);
			printf("  Distance to sphere 1 is %g (radius %g)\n", vnorm(vdiff(o1, v0)), r0);
			printf("  Distance to sphere 2 is %g (radius %g)\n", vnorm(vdiff(o1, v1)), r1);
			printf("  Distance to sphere 3 is %g (radius %g)\n", vnorm(vdiff(o1, v2)), r2);
			printf("Solution 2: %g %g %g\n", o2.x, o2.y, o2.z);
			printf("  Distance to sphere 1 is %g (radius %g)\n", vnorm(vdiff(o2, v0)), r0);
			printf("  Distance to sphere 2 is %g (radius %g)\n", vnorm(vdiff(o2, v1)), r1);
			printf("  Distance to sphere 3 is %g (radius %g)\n", vnorm(vdiff(o2, v2)), r2);

			QLineF line(o1.x, o1.y, o2.x, o2.y);
			userPoly << line.p1() << line.p2();

			goodPoint = line.pointAt(0.5);
			

			if(renderImage)
			{
				p->save();
	
				#ifdef VERBOSE_USER_GRAPHICS
				p->setPen(QPen(Qt::gray, 1. * m_pixelsPerFoot));
				p->setBrush(QColor(0,0,0,127));
				p->drawEllipse(goodPoint, 2 * m_pixelsPerFoot, 2* m_pixelsPerFoot);
	
				//qDrawTextO((*p), (int)goodPoint.x(), (int)goodPoint.y(), QString("%1 - %2 (%3/%4) [b]").arg(info0->essid).arg(info1->essid).arg(i).arg(j));
				#endif
	
				p->restore();
			}

			avgPoint += goodPoint;
			count ++;
		}

	}
	else
	{
		qDebug() << "[user locate] Less than 3 aps visible, cant trilaterate";
	}
#endif
#if 1
// 	if(numAps < 3)
// 	{
// 		qDebug() << "[user locate] Unable to locate, less than 3 APs visible";
// 		m_userItem->setVisible(false);
// 		return;
// 	}

	numAps = qMin(numAps, 3); // only consider the first 3 APs if more than 3

	bool needFirstGoodPoint = false;
	if(numAps == 2)
	{
		QString ap0 = apsVisible[0];
		QString ap1 = apsVisible[1];
	
		avgPoint = triangulate(ap0, apMacToDbm[ap0],
				       ap1, apMacToDbm[ap1]);
		count = 1;
		
		double r0 = dBmToDistance(apMacToDbm[ap0], ap0, rxGain) * m_pixelsPerMeter;
		double r1 = dBmToDistance(apMacToDbm[ap1], ap1, rxGain) * m_pixelsPerMeter;

		
		if(m_locationCheats.contains(ap0))
			r0 = m_locationCheats[ap0];
		if(m_locationCheats.contains(ap1))
			r1 = m_locationCheats[ap1];
		
		QColor color0 = baseColorForAp(ap0);
		QColor color1 = baseColorForAp(ap1);

		#ifdef VERBOSE_USER_GRAPHICS
		if(renderImage)
		{
			MapApInfo *info0 = apInfo(ap0);
			MapApInfo *info1 = apInfo(ap1);
			
// 			updateDrivedLossFactor(info0);
// 			updateDrivedLossFactor(info1);

			QPointF p0 = info0->point;
			QPointF p1 = info1->point;


			// Render the estimated circles covered by these APs
			if(!drawnFlag.contains(ap0))
			{
				drawnFlag.insert(ap0, true);

				p->setPen(QPen(color0, penWidth));

				if(isnan(r0))
					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<"->"<<ap1<<": - Can't render ellipse p0/r0 - radius 0 is NaN";
				else
					p->drawEllipse(p0, r0, r0);
			}

			if(!drawnFlag.contains(ap1))
			{
				drawnFlag.insert(ap1, true);

				p->setPen(QPen(color1, penWidth));

				if(isnan(r1))
					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<"->"<<ap1<<": - Can't render ellipse p0/r0 - radius 1 is NaN";
				else
					p->drawEllipse(p1, r1, r1);

			}
		}
		#endif
	}
	else
	for(int i=0; i<numAps; i++)
	{
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
			
			updateDrivedLossFactor(info0);
			updateDrivedLossFactor(info1);

			QPointF p0 = info0->point;
			QPointF p1 = info1->point;

			qDebug() << "[user locate] testing ("<<i<<"/"<<j<<"): essids:"<<info0->essid<<qPrintable(ap0.right(6))<<"/"<<info1->essid<<qPrintable(ap1.right(6));

			// Not really used - unneeded really since we're using the circle_circle_intersection
			// routine below
			QPointF calcPoint/* = triangulate(ap0, apMacToDbm[ap0],
							ap1, apMacToDbm[ap1])*/;

// 			double r0 = dBmToDistance(apMacToDbm[ap0], ap0, rxGain) * m_pixelsPerMeter;
// 			double r1 = dBmToDistance(apMacToDbm[ap1], ap1, rxGain) * m_pixelsPerMeter;

			double d0 = dBmToDistance(apMacToDbm[ap0], ap0, rxGain);
			double d1 = dBmToDistance(apMacToDbm[ap1], ap1, rxGain);
			double r0 = d0 * m_pixelsPerMeter;
			double r1 = d1 * m_pixelsPerMeter;
			
			double t0 = QLineF(truthTestPoint, p0).length() / m_pixelsPerMeter;
			double t1 = QLineF(truthTestPoint, p1).length() / m_pixelsPerMeter;

// 			double t0 = QLineF(truthTestPoint, p0).length();
// 			double t1 = QLineF(truthTestPoint, p1).length();

			double e0 = d0 - t0;
			double e1 = d1 - t1;

// 			double e0 = r0 - t0;
// 			double e1 = r1 - t1;

			qDebug() << "[user locate] \t dbms: "<<apMacToDbm[ap0]<<" / "<<apMacToDbm[ap1];
			qDebug() << "[user locate] \t dist: "<<d0<<" / "<<d1;
			//qDebug() << "[user locate] \t pixs: "<<r0<<" / "<<r1;
			qDebug() << "[user locate] \t true: "<<t0<<" / "<<t1;
			qDebug() << "[user locate] \t errs: "<<e0<<" / "<<e1;
			qDebug() << "[user locate] \t pnts: "<<p0<<" / "<<p1;
			qDebug() << "[user locate] \t truthTestPoint: "<<truthTestPoint;
			
			if(m_locationCheats.contains(ap0))
				r0 = m_locationCheats[ap0];
			if(m_locationCheats.contains(ap1))
				r1 = m_locationCheats[ap1];
			

// 			QPointF lossModel(2.7, 2.9);
// 			double r0 = dBmToDistance(apMacToDbm[ap0], lossModel, -60, info0->txPower, info0->txGain, rxGain) * m_pixelsPerMeter;
// 			double r1 = dBmToDistance(apMacToDbm[ap1], lossModel, -60, info1->txPower, info1->txGain, rxGain) * m_pixelsPerMeter;

			//n = dBm < -60 ? 2.7 : 1.75;
			
			if(distOverride.contains(ap0))
			       r0 = distOverride[ap0];

			if(distOverride.contains(ap1))
			       r1 = distOverride[ap1];

			double dist = QLineF(p1,p0).length();

// 			if(dist > r0 + r1 ||
// 			   dist < fabs(r0 - r1))
// 			{
// 				// If d > r0 + r1 then there are no solutions, the circles are separate.
// 				// If d < |r0 - r1| then there are no solutions because one circle is contained within the other.
// 		
// 				// Logic tells us that since both signals were observed by the user in the same reading, they must intersect,
// 				// therefore the solution given by dBmToDistance() must be wrong.
// 
// 				// Therefore, we will adjust the lossFactor for these APs inorder to provide
// 				// an intersection by allocation part of the error to each AP
// 
// 				double errorDist = dist > r0 + r1 ? dist - (r0 + r1) : fabs(r0 - r1);
// 
// 				double correctR0 = (r0 + errorDist*.6) / m_pixelsPerMeter;
// 				double correctR1 = (r1 + errorDist*.6) / m_pixelsPerMeter;
// 
// 				double absLossFactor0 = deriveLossFactor(ap0, apMacToDbm[ap0], correctR0 /*, gRx*/);
// 				double absLossFactor1 = deriveLossFactor(ap1, apMacToDbm[ap1], correctR1 /*, gRx*/);
// 
// 				if(isnan(absLossFactor0))
// 				{
// 					//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<": Unable to correct (d>r0+r1), received NaN loss factor, corretR0:"<<correctR0<<", absLossFactor0:"<<absLossFactor0;
// 				}
// 				else
// 				{
// 					QPointF lossFactor = info0->lossFactor;
// 					if(apMacToDbm[ap0] > info0->shortCutoff)
// 						lossFactor.setY(absLossFactor0);
// 					else
// 						lossFactor.setX(absLossFactor0);
// 
// 					info0->lossFactor = lossFactor;
// 
// 					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<": Corrected loss factor for ap0:" <<lossFactor<<", correctR0:"<<correctR0<<", absLossFactor0:"<<absLossFactor0;
// 				}
// 
// 				if(isnan(absLossFactor1))
// 				{
// 					//qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap1<<": Unable to correct (d>r0+r1), received NaN loss factor, corretR1:"<<correctR0<<", absLossFactor0:"<<absLossFactor0;
// 				}
// 				else
// 				{
// 					QPointF lossFactor = info1->lossFactor;
// 					if(apMacToDbm[ap1] > info1->shortCutoff)
// 						lossFactor.setY(absLossFactor1);
// 					else
// 						lossFactor.setX(absLossFactor1);
// 
// 					info0->lossFactor = lossFactor;
// 
// 					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap1<<": Corrected loss factor for ap1:" <<lossFactor<<", correctR1:"<<correctR1<<", absLossFactor:"<<absLossFactor1;
// 				}
// 
// 				// Recalculate distances
// 				r0 = dBmToDistance(apMacToDbm[ap0], ap0) * m_pixelsPerMeter;
// 				r1 = dBmToDistance(apMacToDbm[ap1], ap1) * m_pixelsPerMeter;


#if 1
			//const double overlap = 0.55;
			if(dist > r0 + r1)
			{
// 				// If d > r0 + r1 then there are no solutions, the circles are separate.
				// Distance still wrong, so force-set the proper distance
				double errorDist = dist - (r0 + r1);

// 				if(!distOverride.contains(ap0) &&
// 				   !distOverride.contains(ap1))
// 				{
//
// 					r0 += errorDist * overlap; // overlay a bit
// 					r1 += errorDist * overlap;
//
// 					distOverride[ap0] = r0;
// 					distOverride[ap1] = r1;
//
// 					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): force-corrected the radius [case 0.0]: "<<r0<<r1<<", errorDist: "<<errorDist;
// 				}
// 				else
// 				if(!distOverride.contains(ap0))
// 				{
// 					r0 += errorDist * 1.1;
// 					distOverride[ap0] = r0;
// 					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): force-corrected the radius [case 1.0]: "<<r0<<r1<<", errorDist: "<<errorDist;
// 				}
// 				else
				if(!distOverride.contains(ap1))
				{
					//errorDist = fabs(r1 - r0) * (r1 > r0 ? -1 : 1) + dist;
					r1 += errorDist + 0.001;
					
					distOverride[ap1] = r1;
					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): force-corrected the radius [case 2.0]: "<<r0<<r1<<", errorDist: "<<errorDist;
				}
				else
				{
					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): *Unable* to force-correct the radius [case 3.0]: "<<r0<<r1<<", errorDist: "<<errorDist<<", intersect will fail";
				}
				
			}
			else
			if(dist < fabs(r0 - r1))
			{
// 				// If d < |r0 - r1| then there are no solutions because one circle is contained within the other.
				// Distance still wrong, so force-set the proper distance
				double errorDist = fabs(r0 - r1) - dist;

// 				if(!distOverride.contains(ap0) &&
// 				   !distOverride.contains(ap1))
// 				{
// 					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): values prior to correction[case 0.1]: "<<r0<<r1<<", errorDist: "<<errorDist;
//
// 					if(r0 > r1)
// 					{
// 						r0 -= errorDist * overlap; // overlay a bit
// 						r1 += errorDist * overlap;
// 					}
// 					else
// 					{
// 						r0 += errorDist * overlap; // overlay a bit
// 						r1 -= errorDist * overlap;
//
// 					}
//
// 					distOverride[ap0] = r0;
// 					distOverride[ap1] = r1;
//
// 					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): force-corrected the radius [case 0.1]: "<<r0<<r1<<", errorDist: "<<errorDist;
// 				}
// 				else
// 				if(!distOverride.contains(ap0))
// 				{
// 					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): values prior to correction[case 1.1]: "<<r0<<r1<<", errorDist: "<<errorDist;
//
// 					r0 += (r0 > r1 ? -1:+1) * errorDist * 1.1;
// 					distOverride[ap0] = r0;
//
// 					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): force-corrected the radius [case 1.1]: "<<r0<<r1<<", errorDist: "<<errorDist;
// 				}
//  				else
				if(!distOverride.contains(ap1))
				{
					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): values prior to correction[case 2.1]: "<<r0<<r1<<", errorDist: "<<errorDist;
					
					r1 += (r0 > r1 ? +1:-1) * errorDist * 1.1;
					distOverride[ap1] = r1;

					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): force-corrected the radius [case 2.1]: "<<r0<<r1<<", errorDist: "<<errorDist;
				}
				else
				{
					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): *Unable* to force-correct the radius [case 3.1]: "<<r0<<r1<<", errorDist: "<<errorDist<<", intersect will fail";
				}
					
			}
#endif			
			//}


			QColor color0 = baseColorForAp(ap0);
			QColor color1 = baseColorForAp(ap1);

			#ifdef VERBOSE_USER_GRAPHICS
			if(renderImage)
			{
				// Render the estimated circles covered by these APs
				if(!drawnFlag.contains(ap0))
				{
					drawnFlag.insert(ap0, true);
	
					p->setPen(QPen(color0, penWidth));
	
					if(isnan(r0))
						qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<"->"<<ap1<<": - Can't render ellipse p0/r0 - radius 0 is NaN";
					else
						p->drawEllipse(p0, r0, r0);
				}
	
				if(!drawnFlag.contains(ap1))
				{
					drawnFlag.insert(ap1, true);
	
					p->setPen(QPen(color1, penWidth));
	
					if(isnan(r1))
						qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<"->"<<ap1<<": - Can't render ellipse p0/r0 - radius 1 is NaN";
					else
						p->drawEllipse(p1, r1, r1);
	
				}
			}
			#endif

			qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [circle:pre] i:"<<i<<", r0:"<<r0<<", r1:"<<r1;

			// The revised idea here is this:
			// Need at least three APs to find AP with intersection of circles:
			// Get the two lines the APs intersect
			// Store the first two points at into firstSet
			// Next set, the next two points compared to first two - the two closest go into the goodPoints set, the rejected ones go are 'bad points'
			// From there, the next (third) AP gets compared to goodPoints - the point closest goes into goodPoints, etc
			// At end, good points forms the probability cluster of where the user probably is

			qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [circle:pre] goodPoints:"<<goodPoints<<", idx:"<<i<<", numAps:"<<numAps;

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

			qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [circle:pre] line: "<<line;

			if(line.p1().isNull() || line.p2().isNull())
			{
				qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [circle:pre] line is Null";
				continue;
			}
			else
			{
				#ifdef VERBOSE_USER_GRAPHICS
				if(renderImage)
				{
					p->setPen(QPen(Qt::gray, penWidth));
					p->drawLine(line);
				}
				#endif

				if(goodPoints.isEmpty())
				{
					if(firstSet.p1().isNull() || firstSet.p2().isNull())
					{
						firstSet = line;
						// If only intersecting two APs, then just just give the center of the line.
						// Otherwise, leave blank and let the code below handle it the first time
						goodPoint = numAps > 2 ? QPointF() : line.pointAt(.5);

						qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [circle:step1] firstSet:"<<firstSet<<", goodPoint:"<<goodPoint<<", numAps:"<<numAps;
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

						goodPoint = good.p2();

						qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [circle:step2] firstSet:"<<firstSet<<", line:"<<line<<", goodPoints:"<<goodPoints;
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
						goodPoints << line.p1();
						goodPoint   = line.p1();

						qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [circle:step3] 1<2 ("<<goodAvg1<<":"<<goodAvg2<<"): line was: "<<line<<", goodPoints:"<<goodPoints;
					}
					else
					{
						goodPoints << line.p2();
						goodPoint   = line.p2();

						qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): [circle:step3] 2<1 ("<<goodAvg1<<":"<<goodAvg2<<"): line was: "<<line<<", goodPoints:"<<goodPoints;
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

					qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<"->"<<ap1<<": Point:" <<tmpPoint << " (from last intersection)";
					
					if(renderImage)
					{
	
						p->save();
	
			// 			p->setPen(QPen(color0, penWidth));
			// 			p->drawLine(p0, calcPoint);
			//
			// 			p->setPen(QPen(color1, penWidth));
			// 			p->drawLine(p1, calcPoint);
	
						#ifdef VERBOSE_USER_GRAPHICS
						p->setPen(QPen(Qt::gray, 1. * m_pixelsPerFoot));
						p->setBrush(QColor(0,0,0,127));
						p->drawEllipse(tmpPoint, 2 * m_pixelsPerFoot, 2* m_pixelsPerFoot);
	
						qDrawTextO((*p), (int)tmpPoint.x(), (int)tmpPoint.y(), QString("%1 - %2 (%3/%4) [a]").arg(info0->essid).arg(info1->essid).arg(i).arg(j));
						#endif
	
						p->restore();
					}
					
					avgPoint += tmpPoint;
					count ++;
				}

				calcPoint = goodPoint;

				if(renderImage)
				{
					if(isnan(calcPoint.x()) || isnan(calcPoint.y()))
					{
					//	qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<"->"<<ap1<<": - Can't render ellipse - calcPoint is NaN";
					}
					else
					{
						qDebug() << "MapGraphicsScene::updateUserLocationOverlay(): "<<ap0<<"->"<<ap1<<": Point:" <<calcPoint;
						//qDebug() << "\t color0:"<<color0.name()<<", color1:"<<color1.name()<<", r0:"<<r0<<", r1:"<<r1<<", p0:"<<p0<<", p1:"<<p1;
	
						userPoly << goodPoint;
	
						p->save();
	
			// 			p->setPen(QPen(color0, penWidth));
			// 			p->drawLine(p0, calcPoint);
			//
			// 			p->setPen(QPen(color1, penWidth));
			// 			p->drawLine(p1, calcPoint);
	
						#ifdef VERBOSE_USER_GRAPHICS
						p->setPen(QPen(Qt::gray, 1. * m_pixelsPerFoot));
						p->setBrush(QColor(0,0,0,127));
						p->drawEllipse(goodPoint, 2 * m_pixelsPerFoot, 2* m_pixelsPerFoot);
	
						qDrawTextO((*p), (int)goodPoint.x(), (int)goodPoint.y(), QString("%1 - %2 (%3/%4) [b]").arg(info0->essid).arg(info1->essid).arg(i).arg(j));
						#endif
	
						p->restore();
					}
				}

				if(isnan(calcPoint.x()) || isnan(calcPoint.y()))
				{
					avgPoint += calcPoint;
					count ++;
				}
			}

			//p->setPen(QPen(Qt::gray, penWidth));
			//p->drawLine(p0, p1);

			//break;
		}
	}

#endif

// 	avgPoint.setX( avgPoint.x() / count );
// 	avgPoint.setY( avgPoint.y() / count );
	qDebug() << "[user locate] pre divide: avgPoint:"<<avgPoint<<", count:"<<count;
	avgPoint /= count;

	if(renderImage)
	{
		{
			p->save();
			p->setPen(QPen(Qt::black, 5));
			p->setBrush(QColor(0,0,255,127));
			p->drawPolygon(userPoly);
	
			if(userPoly.size() >= 2)
			{
				double lenSum=0;
				int lenCount=0;
				for(int i=1; i<userPoly.size(); i++)
				{
					QLineF line(userPoly[i-1], userPoly[i]);
	
					lenSum += line.length();
					lenCount ++;
				}
	
				lenSum /= lenCount;
				qDebug() << "avg len: "<<lenSum;
			}
			
			p->restore();
			
			
		}

		penWidth = 20;
	
	// 	p->setPen(QPen(Qt::green, 10.));
	// 	p->setBrush(QColor(0,0,0,127));
	// 	if(!isnan(avgPoint.x()) && !isnan(avgPoint.y()))
	// 		p->drawEllipse(avgPoint, penWidth, penWidth);
	
		if(!isnan(avgPoint.x()) && !isnan(avgPoint.y()))
		{
			p->setPen(QPen(Qt::red, 2. * m_pixelsPerFoot));
	
			p->setBrush(QColor(0,0,0,127));
			p->drawEllipse(avgPoint, penWidth, penWidth);
	
			#ifdef OPENCV_ENABLED
			m_kalman.predictionUpdate((float)avgPoint.x(), (float)avgPoint.y());
	
			float x = avgPoint.x(), y = avgPoint.y();
			m_kalman.predictionReport(x, y);
			QPointF thisPredict(x,y);
	
			p->setPen(QPen(Qt::blue, 2. * m_pixelsPerFoot));
	
			p->setBrush(QColor(0,0,0,127));
			p->drawEllipse(thisPredict, penWidth, penWidth);
			
			avgPoint = thisPredict;
			#endif
		}
		
		if(!truthTestPoint.isNull())
		{
			QLineF errorLine(truthTestPoint, avgPoint);
			
			p->setPen(QPen(Qt::cyan, m_pixelsPerFoot));
			p->drawLine(errorLine);
			
			p->setPen(QPen(Qt::yellow, 2. * m_pixelsPerFoot));
	
			p->setBrush(QColor(0,0,0,127));
			p->drawEllipse(truthTestPoint, penWidth, penWidth);
			
			qDebug() << "[user locate] avgPoint:"<<avgPoint<<", truthTestPoint:"<<truthTestPoint<<", errorLine length:"<<errorLine.length();
			
	
			
		}
	}
	
	m_userLocation = avgPoint;


/*
	p->setPen(QPen(Qt::blue,penWidth));
// 	QPointF s1 = line.p1() - itemWorldRect.topLeft();
// 	QPointF s2 = line.p2() - itemWorldRect.topLeft();
	QPointF s1 = line.p1();
	QPointF s2 = line.p2();

	//p->drawLine(s1,s2);

	QImage user(tr(":/data/images/%1/stock-media-rec.png").arg(size));
	p->drawImage((s1+s2)/2., user.scaled(128,128));

	p->setBrush(Qt::blue);
// 	p->drawRect(QRectF(s1.x()+3,s1.y()+3,6,6));
// 	p->drawRect(QRectF(s2.x()-3,s2.y()-3,6,6));

	p->setBrush(QBrush());
	//p->drawEllipse(center + QPointF(5.,5.), center.x(), center.y());
	p->setPen(QPen(Qt::white,penWidth));
// 	p->drawEllipse(p0, r0,r0);
	p->drawEllipse(p0, la,la);
	p->setPen(QPen(Qt::red,penWidth));
// 	p->drawEllipse(p1, r1,r1);
	p->drawEllipse(p1, lb,lb);
	p->setPen(QPen(Qt::green,penWidth));
	if(!p2.isNull())
		p->drawEllipse(p2, r2,r2);

	p->setPen(QPen(Qt::white,penWidth));
	p->drawRect(QRectF(p0-QPointF(5,5), QSizeF(10,10)));
	p->setPen(QPen(Qt::red,penWidth));
	p->drawRect(QRectF(p1-QPointF(5,5), QSizeF(10,10)));
	p->setPen(QPen(Qt::green,penWidth));
	if(!p2.isNull())
		p->drawRect(QRectF(p2-QPointF(5,5), QSizeF(10,10)));

// 	p->setPen(QPen(Qt::red,penWidth));
// 	p->drawLine(line);
// 	p->drawLine(line2);
// 	p->drawLine(line3);

	penWidth *=2;

	p->setPen(QPen(Qt::white,penWidth));
	p->drawLine(realLine);
	p->setPen(QPen(Qt::gray,penWidth));
	p->drawLine(realLine2);

	penWidth /=2;
	p->setPen(QPen(Qt::darkGreen,penWidth));
	p->drawLine(apLine);
	p->setPen(QPen(Qt::darkYellow,penWidth));
	p->drawLine(userLine);
	p->setPen(QPen(Qt::darkRed,penWidth));
	p->drawLine(userLine2);

// 	p->setPen(QPen(Qt::darkYellow,penWidth));
// 	p->drawRect(QRectF(calcPoint-QPointF(5,5), QSizeF(10,10)));
// 	p->setPen(QPen(Qt::darkGreen,penWidth));
// 	p->drawRect(QRectF(realPoint-QPointF(5,5), QSizeF(10,10)));

*/
	if(renderImage)
	{
		p->end();
	
		m_userItem->setPixmap(QPixmap::fromImage(image));
		//m_userItem->setOffset(-(((double)image.width())/2.), -(((double)image.height())/2.));
		//m_userItem->setPos(itemWorldRect.center());
		//m_userItem->setOffset(offset);
		m_userItem->setOffset(-origSize.width()/2, -origSize.height()/2);
		m_userItem->setPos(0,0);
	
	// 	m_userItem->setOffset(0,0); //-(image.width()/2), -(image.height()/2));
	// 	m_userItem->setPos(0,0); //itemWorldRect.center());
	
		m_userItem->setVisible(true);
		
	}
	
	return image;

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
	
	double invalidRadiusSize = sqrt(origSize.width()*2 + origSize.height()*2);


	#ifdef Q_OS_ANDROID
	QString size = "64x64";
	#else
	QString size = "32x32";
	#endif

	double penWidth = 1.0;

	QHash<QString,bool> drawnFlag;
	QHash<QString,int> badLossFactor;

	QVector<QPointF> userPoly;

	QPointF avgPoint(0.,0.);
	int count = 0;

	QHash<QString, QList<SigMapValue*> > valuesByAp;
	foreach(QString apMac, m_apInfo.keys())
	{
		MapApInfo *info = apInfo(apMac);
		if(!info->renderOnMap)
			continue;
			
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


		qDebug() << "[apLocationGuess] Found "<<list.size()<<" readings for apMac:"<<apMac<<", ESSID:" << info->essid;

		if(list.isEmpty())
			continue;

		QColor color0 = baseColorForAp(apMac);


		QPointF avgPoint(0.,0.);
		count = 0;
		
		
		// The "best guess" at the "correct" intersection of the circles
		QList<QPointF> goodPoints;
		QLineF firstSet;
	
		QStringList pairsTested;
	
		int numVals = list.size();
		bool needFirstGoodPoint = false;

		//SigMapValue *val0 = list.first();

// 		QPointF strongestReading = numVals > 0 ? list[0]->point : QPointF();
		

		QFont font = p.font();
		font.setPointSizeF(6 * m_pixelsPerFoot);
		p.setFont(font);

// 		for(int i=0; i<numVals; i++)
		for(int i=0; i<1; i++)
		{
			QPointF strongestReading;
			for(int j=0; j<numVals; j++)
			{
				QString key = QString("%1%2").arg(i<j?i:j).arg(i<j?j:i);
				if(i == j || pairsTested.contains(key))
				{
					//qDebug() << "\t not testing ("<<i<<"/"<<j<<"): key:"<<key;
					continue;
				}
				
				pairsTested << key;
				
				//SigMapValue *val1 = list[i];
				SigMapValue *val0 = list[i];
				SigMapValue *val1 = list[j];
				
				if(strongestReading.isNull())
					strongestReading = val0->point;
				
				double value0 = val0->signalForAp(apMac);
				double value1 = val1->signalForAp(apMac);
				
				qDebug() << "AP Intersect: Processing: "<<i<<", "<<j<<", sig[i]:"<<value0<<", sig[j]:"<<value1;
				
				// For sake of speed on large data sets, 
				// don't use points that had less than 40% signal reading
				if(value0 < 0.1 ||
				   value1 < 0.1)
				   continue;
				
				QPointF p0 = val0->point;
				QPointF p1 = val1->point;
				
				QPointF calcPoint;
				
				int dbm0 = (int)val0->signalForAp(apMac, true);
				int dbm1 = (int)val1->signalForAp(apMac, true);
				
				// We're assuming default loss factors for this AP
				double r0 = dBmToDistance(dbm0, apMac) * m_pixelsPerMeter;
				double r1 = dBmToDistance(dbm1, apMac) * m_pixelsPerMeter;
				
				if(r0 < 1.0 || r0 > invalidRadiusSize) // loss factor is invalid
				{
					QPointF lossFactor = info->lossFactor;
					if(dbm0 > info->shortCutoff)
						lossFactor.setY(2.);
					else
						lossFactor.setX(4.);

					info->lossFactor = lossFactor;
					
					//qDebug() << "MapGraphicsScene::updateApLocationOverlay(): "<<apMac<<": Invalid loss factor r0, corrected:" <<lossFactor;
					
					// Recalculate distances
					r0 = dBmToDistance(dbm0, apMac) * m_pixelsPerMeter;
				}
				
				if(r1 < 1.0 || r1 > invalidRadiusSize) // loss factor is invalid
				{
					QPointF lossFactor = info->lossFactor;
					if(dbm1 > info->shortCutoff)
						lossFactor.setY(2.);
					else
						lossFactor.setX(4.);

					info->lossFactor = lossFactor;
					
					//qDebug() << "MapGraphicsScene::updateApLocationOverlay(): "<<apMac<<": Invalid loss factor r1, corrected:" <<lossFactor;
					
					// Recalculate distances
					r1 = dBmToDistance(dbm1, apMac) * m_pixelsPerMeter;
				}
				
				double dist = QLineF(p1,p0).length();
				
// 				if(dist > r0 + r1 ||
// 				   dist < fabs(r0 - r1))
// 				{
// 					// If d > r0 + r1 then there are no solutions, the circles are separate.
// 					// If d < |r0 - r1| then there are no solutions because one circle is contained within the other.
// 			
// 					// Logic tells us that since both signals were observed by the user in the same reading, they must intersect,
// 					// therefore the solution given by dBmToDistance() must be wrong.
// 	
// 					// Therefore, we will adjust the lossFactor for these APs inorder to provide
// 					// an intersection by allocation part of the error to each AP
// 	
// 					double errorDist = dist > r0 + r1 ? dist - (r0 + r1) : fabs(r0 - r1);
// 	
// 					double corrected = (r0 + errorDist * .6) / m_pixelsPerMeter;
// 					
// 					double absLossFactor = deriveLossFactor(apMac, dbm0, corrected/*, gRx*/);
// 					
// 					if(isnan(absLossFactor))
// 					{
// 						//qDebug() << "MapGraphicsScene::updateApLocationOverlay(): "<<ap0<<": Unable to correct (d>r0+r1), received NaN loss factor, corretR0:"<<correctR0<<", absLossFactor0:"<<absLossFactor0;
// 					}
// 					else
// 					{
// 						QPointF lossFactor = info->lossFactor;
// 						if(dbm0 > info->shortCutoff)
// 							lossFactor.setY(absLossFactor);
// 						else
// 							lossFactor.setX(absLossFactor);
// 	
// 						info->lossFactor = lossFactor;
// 	
// 						qDebug() << "MapGraphicsScene::updateApLocationOverlay(): "<<apMac<<": Corrected loss factor for apMac:" <<lossFactor<<", corrected:"<<corrected<<", absLossFactor:"<<absLossFactor;
// 					}
//
					
					// Recalculate distances
					r0 = dBmToDistance(dbm0, apMac) * m_pixelsPerMeter;
					r1 = dBmToDistance(dbm1, apMac) * m_pixelsPerMeter;
	
					if(dist > r0 + r1)
					{
						// Allocate distance to the secondary radius
						double errorDist = dist - (r0 + r1);

						r1 += errorDist;// * 1.1;

						qDebug() << "MapGraphicsScene::updateApLocationOverlay(): force-corrected the radius [case 0]: "<<r0<<r1<<", errorDist: "<<errorDist;

					}
					else
					if(dist < fabs(r0 - r1))
					{
						// Distance still wrong, so force-set the proper distance
						double errorDist = fabs(r0 - r1) - dist;
						r1 += (r0 > r1 ? +1:-1) * errorDist * 1.1;
						
						qDebug() << "MapGraphicsScene::updateApLocationOverlay(): force-corrected the radius [case 1]: "<<r0<<r1<<", errorDist: "<<errorDist;
					}
				
				//}
				
				// Draw ellipses
    /*
				p.setPen(QPen(darkenColor(color0, value0), penWidth * m_pixelsPerFoot));
 				if(p0.x() > 0 && p0.y() > 0 && r0 > 1)
 					p.ipse(p0, r0, r0);
				p.setPen(QPen(darkenColor(color0, value1), penWidth * m_pixelsPerFoot));
 				if(p1.x() > 0 && p1.y() > 0 && r1 > 1)
 					p.drawEllipse(p1, r1, r1);
				*/

				p.save();
				{
					QFont font = p.font();
					font.setPointSizeF(6 * m_pixelsPerFoot);
					p.setFont(font);
					
					p.setPen(QPen(darkenColor(color0, value0), 3. * m_pixelsPerFoot));
					p.setBrush(QColor(0,0,0,127));
					QPointF randPoint(
						(double)rand()/(double)RAND_MAX * m_pixelsPerFoot*2 - m_pixelsPerFoot,
						(double)rand()/(double)RAND_MAX * m_pixelsPerFoot*2 - m_pixelsPerFoot
					);
					//qDebug() << "MapGraphicsScene::updateApLocationOverlay(): [apLocationGuess] ap:"<<apMac<<", drawing p0 at:"<<(p0 + randPoint);
					p.drawEllipse(p0 + randPoint, 2 * m_pixelsPerFoot, 2* m_pixelsPerFoot);
					//qDrawTextO(p, (int)p0.x(), (int)p0.y(), QString().sprintf("%d%% #%d", (int)(value0*100), i));

					p.setPen(QPen(darkenColor(color0, value1), 3. * m_pixelsPerFoot));
					p.setBrush(QColor(0,0,0,127));
					randPoint = QPointF(
						(double)rand()/(double)RAND_MAX * m_pixelsPerFoot*2 - m_pixelsPerFoot,
						(double)rand()/(double)RAND_MAX * m_pixelsPerFoot*2 - m_pixelsPerFoot
					);
					//qDebug() << "MapGraphicsScene::updateApLocationOverlay(): [apLocationGuess] ap:"<<apMac<<", drawing p1 at:"<<(p1 + randPoint);
					p.drawEllipse(p1 + randPoint, 2 * m_pixelsPerFoot, 2 * m_pixelsPerFoot);
					//qDrawTextO(p, (int)p1.x(), (int)p1.y(), QString().sprintf("%d%% #%d", (int)(value1*100), j));
				}
				p.restore();
				
				
				// The revised idea here is this:
				// Need at least three APs to find AP with intersection of circles:
				// Get the two lines the APs intersect
				// Store the first two points at into firstSet
				// Next set, the next two points compared to first two - the two closest go into the goodPoints set, the rejected ones go are 'bad points'
				// From there, the next (third) AP gets compared to goodPoints - the point closest goes into goodPoints, etc
				// At end, good points forms the probability cluster of where the user probably is
	
				//qDebug() << "MapGraphicsScene::updateApLocationOverlay(): [circle:pre] #goodPoints:"<<goodPoints.size()<<", i:"<<i<<",j:"<<j<<", numVals:"<<numVals<<", p0:"<<p0<<", r0:"<<r0<<", p1:"<<p1<<", r1:"<<r1;
	
// 				qDebug() << " -test done-";
// 				exit(-1);
				
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
					qDebug() << "MapGraphicsScene::updateApLocationOverlay(): circle_circle_intersection() returned 0";
					continue;
				}
	
				QLineF line(xi, yi, xi_prime, yi_prime);

				if(isnan(xi))
					qDebug() << "MapGraphicsScene::updateApLocationOverlay(): circle_circle_intersection(): xi is NaN";
				if(isnan(yi))
					qDebug() << "MapGraphicsScene::updateApLocationOverlay(): circle_circle_intersection(): yi is NaN";
				if(isnan(xi_prime))
					qDebug() << "MapGraphicsScene::updateApLocationOverlay(): circle_circle_intersection(): xi_prime is NaN";
				if(isnan(yi_prime))
					qDebug() << "MapGraphicsScene::updateApLocationOverlay(): circle_circle_intersection(): yi_prime is NaN";
	
	
				if(line.isNull())
				{
					continue;
				}
				else
				{
					//p.setPen(QPen(Qt::gray, penWidth));
					p.setPen(QPen(color0, penWidth));
					//p.drawLine(line);
	
					if(goodPoints.isEmpty())
					{
						if(firstSet.isNull())
						{
							firstSet = line;
							// If only intersecting two APs, then just just give the center of the line.
							// Otherwise, leave blank and let the code below handle it the first time
							goodPoint = numVals > 2 ? QPointF() : (line.p1() + line.p2()) / 2;
	
							//qDebug() << "MapGraphicsScene::updateApLocationOverlay(): [circle:step1] firstSet:"<<firstSet<<", goodPoint:"<<goodPoint<<", numVals:"<<numVals;
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
	
							goodPoint = good.p2();
	
							//qDebug() << "MapGraphicsScene::updateApLocationOverlay(): [circle:step2] firstSet:"<<firstSet<<", line:"<<line<<", goodPoints:"<<goodPoints;
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
							goodPoints << line.p1();
							goodPoint   = line.p1();
	
							//qDebug() << "MapGraphicsScene::updateApLocationOverlay(): [circle:step3] 1<2 ("<<goodAvg1<<":"<<goodAvg2<<"): line was: "<<line<<", goodPoints:"<<goodPoints;
						}
						else
						{
							goodPoints << line.p2();
							goodPoint   = line.p2();
	
							//qDebug() << "MapGraphicsScene::updateApLocationOverlay(): [circle:step3] 2<1 ("<<goodAvg1<<":"<<goodAvg2<<"): line was: "<<line<<", goodPoints:"<<goodPoints;
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
	
						//qDebug() << "MapGraphicsScene::updateApLocationOverlay(): "<<ap0<<"->"<<ap1<<": Point:" <<tmpPoint << " (from last intersection)";
	
	
						p.save();
	
			// 			p.setPen(QPen(color0, penWidth));
			// 			p.drawLine(p0, calcPoint);
			//
			// 			p.setPen(QPen(color1, penWidth));
			// 			p.drawLine(p1, calcPoint);
	
						p.setPen(QPen(darkenColor(color0, (value0 + value1)/2), 1.0 * m_pixelsPerFoot));
						p.setBrush(QColor(0,0,0,127));
						//qDebug() << "MapGraphicsScene::updateApLocationOverlay(): [apLocationGuess] ap:"<<apMac<<", drawing tmpPoint at:"<<tmpPoint<<", goodPoints:"<<goodPoints;
						//if(tmpPoint.x() > 0 && tmpPoint.y() > 0)
							p.drawEllipse(tmpPoint, 2 * m_pixelsPerFoot, 2 * m_pixelsPerFoot);

						//qDrawTextO(p, (int)tmpPoint.x(), (int)tmpPoint.y(), QString().sprintf("%d%% / %d%% #%d", (int)(value0*100), (int)(value1*100), j));
	
						p.restore();
	
						avgPoint += tmpPoint/* * inverseDistanceWeight(tmpPoint, strongestReading, 2.5)*/;
						count ++;
					}
	
					calcPoint = goodPoint;
	
					if(isnan(calcPoint.x()) || isnan(calcPoint.y()))
					{
					//	qDebug() << "MapGraphicsScene::updateApLocationOverlay(): "<<ap0<<"->"<<ap1<<": - Can't render ellipse - calcPoint is NaN";
					}
					else
					{
						//qDebug() << "MapGraphicsScene::updateApLocationOverlay(): "<<ap0<<"->"<<ap1<<": Point:" <<calcPoint;
						//qDebug() << "\t color0:"<<color0.name()<<", color1:"<<color1.name()<<", r0:"<<r0<<", r1:"<<r1<<", p0:"<<p0<<", p1:"<<p1;
	
						//double w = (value0 + value1) / 2.;
// 						double w = inverseDistanceWeight(calcPoint, strongestReading, 0.1);
// 						qDebug() << "MapGraphicsScene::updateApLocationOverlay(): Point:" <<calcPoint<<", strongest:"<<strongestReading<<", weight:"<<w;
// 						
// 						QLineF baseline(strongestReading, calcPoint);
// 						baseline.setLength(baseline.length() * w);
// 						
// 						//avgPoint += baseline.p2();
// 						calcPoint = baseline.p2();
// 						
// 						p.setPen(QPen(Qt::gray, penWidth * m_pixelsPerFoot));
// 						p.drawLine(baseline);
						
						userPoly << calcPoint;
	
						p.save();

						//qDebug() << "MapGraphicsScene::updateApLocationOverlay(): [apLocationGuess] ap:"<<apMac<<", drawing calcPoint at:"<<calcPoint;
						
						p.setPen(QPen(darkenColor(color0, (value0 + value1)/2), 1. * m_pixelsPerFoot));
						p.setBrush(QColor(0,0,0,127));
						//if(calcPoint.x() > 0 && calcPoint.y() > 0)
							p.drawEllipse(calcPoint, 2 * m_pixelsPerFoot, 2 * m_pixelsPerFoot);

						//qDrawTextO(p, (int)calcPoint.x(), (int)calcPoint.y(), QString().sprintf("%d%% / %d%% #%d", (int)(value0*100), (int)(value1*100), j));
						
	
						p.restore();
	
						avgPoint += calcPoint/* * w*/;
						count ++;
					}
				}
			}

		}

		avgPoint /= count;

		//p.setPen(QPen(Qt::red, 30.));
		//p.drawPolygon(userPoly);
		
		if(0)
		{
			p.save();
			p.setPen(QPen(Qt::black, 5));
			p.setBrush(QColor(0,0,255,127));
			p.drawPolygon(userPoly);
			p.restore();
		}

		penWidth = 5;

// 		p.setPen(QPen(Qt::green, 10.));
// 		p.setBrush(QColor(0,0,0,127));
// 		if(!isnan(avgPoint.x()) && !isnan(avgPoint.y()))
// 			p.drawEllipse(avgPoint, penWidth, penWidth);

		qDebug() << "MapGraphicsScene::updateApLocationOverlay(): [apLocationGuess] ap:"<<apMac<<", avgPoint:"<<avgPoint<<", count:"<<count;

		if(!isnan(avgPoint.x()) && !isnan(avgPoint.y()))
		{
			info->locationGuess = avgPoint;
			
			// marked means user set point, so don't override if user marked
			if(!info->marked)
				info->point = avgPoint;
		}

	}

	foreach(QString apMac, m_apInfo.keys())
	{
		MapApInfo *info = apInfo(apMac);
		if(!info->renderOnMap)
			continue;

		//qDebug() << "MapGraphicsScene::updateApLocationOverlay(): [apLocationGuess] ap:"<<apMac<<", rendering at avgPoint:"<<avgPoint;
		
		QColor color0 = baseColorForAp(apMac);
		
		QPointF avgPoint = info->locationGuess;
		if(!avgPoint.isNull())
		{
			p.setPen(QPen(color0, 10.));
			p.setBrush(QColor(0,0,0,127));

			p.drawEllipse(avgPoint, 12 * m_pixelsPerFoot, 12 * m_pixelsPerFoot);

			QImage markerGroup(":/data/images/ap-marker.png");
			p.drawImage(avgPoint - QPoint(markerGroup.width()/2,markerGroup.height()/2), markerGroup);

			p.drawEllipse(avgPoint, penWidth, penWidth);

			qDrawTextO(p,(int)avgPoint.x(),(int)avgPoint.y(),apInfo(apMac)->essid);
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
	
	if(m_locationCheats.contains(ap0))
		la = m_locationCheats[ap0];
	if(m_locationCheats.contains(ap1))
		lb = m_locationCheats[ap1];
	

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

	QPointF lossFactor(4.,2.);

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
			longFactor = 4.;
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

		double shortFactor = 2.0; // starting factor guesses
		double longFactor  = 4.0; // starting factor guesses

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

	QPointF lossFactor(4.,2.);

	// Use a reworked distance formula to calculate lossFactor for each point, then average together

	double shortFactor = 0.,
		longFactor = 0.;

	int    shortCount  = 0,
		longCount  = 0;

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
		longFactor = 4.;
		longCount  = 1;
	}

	lossFactor = QPointF(    longFactor /  longCount,
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

	//shortCutoff = 40;
	// n is the loss factor describing obstacles, walls, reflection, etc, can be derived from existing data with driveObservedLossFactor()
	double n   = dBm < shortCutoff ? lossFactor.x() : lossFactor.y();
	//n = dBm < -60 ? 2.7 : 1.75;

	double m   =  0.12; // (meters) - wavelength of 2442 MHz, freq of 802.11b/g radio
	double Xa  =  3.00; // (double)rand()/(double)RAND_MAX * 17. + 3.; // normal rand var, std dev a=[3,20]

	double logDist = (1/(10*n)) * (txPower - dBm + txGain + rxGain - Xa + 20*log10(m) - 20*log10(4*Pi));
	double distMeters = pow(10, logDist); // distance in meters

	//qDebug() << "MapGraphicsScene::dBmToDistance(): "<<dBm<<": meters:"<<distMeters<<", Debug: n:"<<n<<", txPower:"<<txPower<<", txGain:"<<txGain<<", rxGain:"<<rxGain<<", logDist:"<<logDist<<", lf:"<<lossFactor<<", sc:"<<shortCutoff;

	return distMeters;
}
