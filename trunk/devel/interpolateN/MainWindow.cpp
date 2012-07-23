#include <QtGui>

#include "SurfaceInterpolate.h"

#define qDrawTextCp(p,pnt,string,c1,c2) 	\
	p.setPen(c1);			\
	p.drawText(pnt + QPoint(-1, -1), string);	\
	p.drawText(pnt + QPoint(+1, -1), string);	\
	p.drawText(pnt + QPoint(+1, +1), string);	\
	p.drawText(pnt + QPoint(-1, +1), string);	\
	p.setPen(c2);			\
	p.drawText(pnt, string);	\
	
#define qDrawTextOp(p,pnt,string) 	\
	qDrawTextCp(p,pnt,string,Qt::black,Qt::white);

class MainWindow : public QLabel {
	public:
		MainWindow();
};

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	MainWindow mw;
	mw.show();
	mw.adjustSize();
	return app.exec();
}

#define fuzzyIsEqual(a,b) ( fabs(a - b) < 0.00001 )
//#define fuzzyIsEqual(a,b) ( ((int) a) == ((int) b) )

namespace SurfaceInterpolate {

/// \brief Simple value->color function
QColor colorForValue(double v)
{
	int hue = qMax(0, qMin(359, (int)((1-v) * 359)));
	//return QColor::fromHsv(hue, 255, 255);
	
	//int hue = (int)((1-v) * (359-120) + 120);
 	//hue = qMax(0, qMin(359, hue));

	//int hue = (int)(v * 120);
	//hue = qMax(0, qMin(120, hue));
 	 
	int sat = 255;
	int val = 255;
	double valTop = 0.9;
	if(v < valTop)
	{
		val = (int)(v/valTop * 255.);
		val = qMax(0, qMin(255, val));
	}
	
	return QColor::fromHsv(hue, sat, val);
}


// The following four cubic interpolation functions are from http://www.paulinternet.nl/?page=bicubic
double cubicInterpolate (double p[4], double x) {
	return p[1] + 0.5 * x*(p[2] - p[0] + x*(2.0*p[0] - 5.0*p[1] + 4.0*p[2] - p[3] + x*(3.0*(p[1] - p[2]) + p[3] - p[0])));
}

double bicubicInterpolate (double p[4][4], double x, double y) {
	double arr[4];
	arr[0] = cubicInterpolate(p[0], y);
	arr[1] = cubicInterpolate(p[1], y);
	arr[2] = cubicInterpolate(p[2], y);
	arr[3] = cubicInterpolate(p[3], y);
	return cubicInterpolate(arr, x);
}

/*
/// Not used:
double tricubicInterpolate(double p[4][4][4], double x, double y, double z) {
	double arr[4];
	arr[0] = bicubicInterpolate(p[0], y, z);
	arr[1] = bicubicInterpolate(p[1], y, z);
	arr[2] = bicubicInterpolate(p[2], y, z);
	arr[3] = bicubicInterpolate(p[3], y, z);
	return cubicInterpolate(arr, x);
}

/// Not used:
double nCubicInterpolate (int n, double* p, double coordinates[]) {
	//assert(n > 0);
	if(n <= 0)
		return -1;
	if (n == 1) {
		return cubicInterpolate(p, *coordinates);
	}
	else {
		double arr[4];
		int skip = 1 << (n - 1) * 2;
		arr[0] = nCubicInterpolate(n - 1, p, coordinates + 1);
		arr[1] = nCubicInterpolate(n - 1, p + skip, coordinates + 1);
		arr[2] = nCubicInterpolate(n - 1, p + 2*skip, coordinates + 1);
		arr[3] = nCubicInterpolate(n - 1, p + 3*skip, coordinates + 1);
		return cubicInterpolate(arr, *coordinates);
	}
}
*/

//bool newQuad = false;
/// \brief Interpolate the value for point \a x, \a y from the values at the four corners of \a quad
double quadInterpolate(qQuadValue quad, double x, double y)
{
	double w = quad.width();
	double h = quad.height();
	
	bool useBicubic = true;
	if(useBicubic)
	{
		// Use bicubic impl from http://www.paulinternet.nl/?page=bicubic
		double unitX = (x - quad.left() ) / w;
		double unitY = (y - quad.top()  ) / h;
		
		double	c1 = quad.tl.value,
			c2 = quad.tr.value,
			c3 = quad.br.value,
			c4 = quad.bl.value;

		double p[4][4] = 
		{
// 			{c1,c1,c2,c2},
// 			{c1,c1,c2,c2},
// 			{c4,c4,c3,c3},
// 			{c4,c4,c3,c3}
			
			// Note: X runs DOWN, values below are (x1,y1)->(x1,y4), NOT (x1,y1)->(x4,y1) as above (commented out) 
			{c1,c1,c4,c4},
			{c1,c1,c4,c4},
			{c2,c2,c3,c3},
			{c2,c2,c3,c3}
		};
		
		double p2 = bicubicInterpolate(p, unitX, unitY);
		//if(p2 > 1.0 || unitX > 1. || unitY > 1. || newQuad)
		//if(newQuad && unitX > 0.99)
		//{
			//qDebug() << "bicubicInterpolate: unit:"<<unitX<<","<<unitY<<": p2:"<<p2<<", w:"<<w<<", h:"<<h<<", x:"<<x<<", y:"<<y<<", corners:"<<corners[0].point<<corners[1].point<<corners[2].point<<corners[3].point<<", v:"<<c1<<c2<<c3<<c4;
			
			//newQuad = false;
		//}
		
		return p2;
	}
	else
	{
		// Very simple implementation of http://en.wikipedia.org/wiki/Bilinear_interpolation
		double fr1 = (quad.br.point.x() - x) / w * quad.tl.value
			   + (x - quad.tl.point.x()) / w * quad.tr.value;
	
		double fr2 = (quad.br.point.x() - x) / w * quad.bl.value
			   + (x - quad.tl.point.x()) / w * quad.br.value;
	
		double p1  = (quad.br.point.y() - y) / h * fr1
			   + (y - quad.tl.point.y()) / h * fr2;
	
		//qDebug() << "quadInterpolate: "<<x<<" x "<<y<<": "<<p<<" (fr1:"<<fr1<<",fr2:"<<fr2<<",w:"<<w<<",h:"<<h<<")";
		return p1;
	}
}

/// \brief Internal comparitor for sorting qPointValues
bool _qPointValue_sort_point(qPointValue a, qPointValue b)
{
	return  fuzzyIsEqual(a.point.y(), (int)b.point.y()) ?
		(int)a.point.x()  < (int)b.point.x() :
		(int)a.point.y()  < (int)b.point.y();
}

/// \brief Checks \a list to see if it contains \a point, \returns true if \a point is in \a list, false otherwise 
bool hasPoint(QList<qPointValue> list, QPointF point)
{
	foreach(qPointValue v, list)
		if(v.point == point)
			return true;
	return false;
}

/*
/// \brief Calculates the weightof point \a o at point \a i given power \a p, used internally by interpolateValue()
double weight(QPointF i, QPointF o, double p)
{
	return 1/pow(QLineF(i,o).length(), p);
}

/// \brief A very simple IDW implementation to interpolate the value at \a point from the inverse weighted distance of all the other points given in \a inputs, using a default power factor 
double interpolateValue(QPointF point, QList<qPointValue> inputs)
{
	double p = 3;
	int n = inputs.size();
	double sum = 0;
	for(int i=0; i<n; i++)
	{
		double a = weight(inputs[i].point, point, p) * inputs[i].value;
		double b = 0;
		for(int j=0; j<n; j++)
			b += weight(inputs[j].point, point, p);
		sum += a/b;
	}
	return sum;
}
*/

/// \brief Calculates the weight of point \a o at point \a i given power \a p, used internally by interpolateValue()
#define _dist2(a, b) ( ( ( a.x() - b.x() ) * ( a.x() - b.x() ) ) + ( ( a.y() - b.y() ) * ( a.y() - b.y() ) ) )
//#define NEARBY2 (2000 * 2000)
//#define NEARBY2 (4000 * 4000)
//#define NEARBY2 (1000 * 1000)

inline double weight(QPointF i, QPointF o, double p)
{
	double b = pow(sqrt(_dist2(i,o)),p);
	return b == 0 ? 0 : 1 / b;
}

/// \brief A very simple IDW implementation to interpolate the value at \a point from the inverse weighted distance of all the other points given in \a inputs, using a default power factor 
double Interpolator::interpolateValue(QPointF point, QList<qPointValue> inputs)
{
	//return 1;
	/*
	http://en.wikipedia.org/wiki/Inverse_distance_weighting:
	u(x) = sum(i=0..N, (
		W(i,x) * u(i)
		----------
		sum(j=0..N, W(j,x))
		)
	
	Where:
	W(i,X) = 1/(d(X,Xi)^p
	*/
	double p = 4;
	int n = inputs.size();
	double sum = 0;
	for(int i=0; i<n; i++)
	{
		qPointValue x = inputs[i];
		if(_dist2(x.point, point) < m_nearbyDistance)
		{
			double a = weight(x.point, point, p) * x.value;
			double b = 0;
			for(int j=0; j<n; j++)
			{
				QPointF y = inputs[j].point;
				if(_dist2(y, point) < m_nearbyDistance)
					b += weight(y, point, p);
			}
			sum += b == 0 ? 0 : a / b;
		}
	}
	return sum;
}


inline int Interpolator::pointKey(QPointF p)
{
	return (int)(p.y() * m_currentBounds.width() + p.x());
}

inline bool Interpolator::isPointUsed(QPointF p)
{
	return m_pointsUsed.contains(pointKey(p));
}

inline void Interpolator::markPointUsed(QPointF p)
{
	int k = pointKey(p);
	if(!m_pointsUsed.contains(k))
		m_pointsUsed.append(k);
}

QList<qPointValue> Interpolator::testLine(QList<qPointValue> inputs, QPointF p1, QPointF p2, int dir)
{
	QRectF bounds = m_currentBounds;
	
	QLineF boundLine(p1,p2);
	QList<qPointValue> outputs;
	
	if(!isPointUsed(boundLine.p1()))
	{
		markPointUsed(boundLine.p1());
		double value = interpolateValue(boundLine.p1(), inputs);
		outputs << qPointValue(boundLine.p1(), value);
	}
	
	if(!isPointUsed(boundLine.p2()))
	{
		markPointUsed(boundLine.p2());
		double value = interpolateValue(boundLine.p2(), inputs);
		outputs << qPointValue(boundLine.p2(), value);;
	}
	
	int count=0, max=inputs.size();
	foreach(qPointValue v, inputs)
	{
		qDebug() << "testLine(): dir:"<<dir<<", processing point:"<<(count++)<<"/"<<max;
		QLineF line;
		if(dir == 1)
			// line from point to TOP
			// (boundLine runs right-left on TOP side of bounds)
			line = QLineF(v.point, QPointF(v.point.x(), bounds.top()));
		else
		if(dir == 2)
			// line from point to RIGHT
			// (boundLine runs top-bottom on RIGHT side of bounds)
			line = QLineF(v.point, QPointF(bounds.right(), v.point.y()));
		else
		if(dir == 3)
			// line from point to BOTTOM
			// (boundLine runs right-left on BOTTOM side of bounds)
			line = QLineF(v.point, QPointF(v.point.x(), bounds.bottom()));
		else
		if(dir == 4)
			// line from point to LEFT
			// (boundLine runs top-bottom on LEFT side of bounds)
			line = QLineF(v.point, QPointF(bounds.left(),  v.point.y()));
		
		// test to see boundLine intersects line, if so, create point if none exists
		QPointF point;
		/*QLineF::IntersectType type = */boundLine.intersect(line, &point);
		if(!point.isNull())
		{
			if(!isPointUsed(point))
			{
				markPointUsed(point);
			
				double value = interpolateValue(point, inputs);
				outputs << qPointValue(point, value);
				//qDebug() << "testLine: "<<boundLine<<" -> "<<line<<" ["<<dir<<"/1] New point: "<<point<<", value:"<<value<<", corners: "<<c1.value<<","<<c2.value;
			}
		}
		
		foreach(qPointValue v2, inputs)
		{
			QLineF line2;
			if(dir == 1 || dir == 3)
				// 1=(boundLine runs right-left on TOP side of bounds)
				// 3=(boundLine runs right-left on BOTTOM side of bounds)
				// run line left right, see if intersects with 'line' above
				line2 = QLineF(bounds.left(), v2.point.y(), bounds.right(), v2.point.y());
			else
			if(dir == 2 || dir == 4)
				// 2=(boundLine runs top-bottom on RIGHT side of bounds)
				// 4=(boundLine runs top-bottom on LEFT side of bounds)
				// run line left right, see if intersects with 'line' above
				line2 = QLineF(v2.point.x(), bounds.top(), v2.point.x(), bounds.bottom());
			
			//qDebug() << "testLine: "<<boundLine<<" -> "<<line<<" ["<<dir<<"/2] Testing: "<<line2;
			
			// test to see boundLine intersects line, if so, create point if none exists
			QPointF point2;
			/*QLineF::IntersectType type = */line.intersect(line2, &point2);
			if(!point2.isNull())
			{
				if(!isPointUsed(point2))
				{
					markPointUsed(point2);
					double value = interpolateValue(point2, inputs);
					outputs << qPointValue(point2, isnan(value) ? 0 :value);
				}
			}
		}
		
	}
	
	
	return outputs;
}

qPointValue nearestPoint(QList<qPointValue> list, QPointF point, int dir, bool requireValid=false)
{
	/// \a dir: 0=X+, 1=Y+, 2=X-, 3=Y-
	
	qPointValue minPnt;
	double minDist = (double)INT_MAX;
	double origin  = dir == 0 || dir == 2 ? point.x() : point.y();

	foreach(qPointValue v, list)
	{
		if(requireValid && !v.isValid())
			continue;
			
		double val  = dir == 0 || dir == 2 ? v.point.x() : v.point.y();
		double dist = fabs(val - origin);
		if(((dir == 0 && fuzzyIsEqual(v.point.y(), point.y()) && val > origin) ||
		    (dir == 1 && fuzzyIsEqual(v.point.x(), point.x()) && val > origin) ||
		    (dir == 2 && fuzzyIsEqual(v.point.y(), point.y()) && val < origin) ||
		    (dir == 3 && fuzzyIsEqual(v.point.x(), point.x()) && val < origin) )  &&
		    dist < minDist)
		{
			minPnt = v;
			minDist = dist;
		}
	}
	
	return minPnt;
}

/// \brief Returns the bounds (max/min X/Y) of \a points
QRectF getBounds(QList<qPointValue> points)
{
	QRectF bounds;
	foreach(qPointValue v, points)
	{
		//qDebug() << "corner: "<<v.point<<", value:"<<v.value;
		bounds |= QRectF(v.point, QSizeF(1.,1.0));
	}
	bounds.adjust(0,0,-1,-1);
	
	return bounds;
}
	


/**
\brief Generates a list of quads covering the every point given by \a points using rectangle bicubic interpolation.

	Rectangle Bicubic Interpolation
	
	The goal:
	Given an arbitrary list of points (and values assoicated with each point),
	interpolate the value for every point contained within the bounding rectangle.
	Goal something like: http://en.wikipedia.org/wiki/File:BicubicInterpolationExample.png
	
	To illustrate what this means and how we do it, assume you give it four points (marked by asterisks, below),
	along with an arbitrary value for each point:
	
	*-----------------------
	|                       |
	|                       |
	|       *               |
	|                       |
	|                       |
	|                       |
	|                       |
	|             *         |
	|                       |
	|                       |
	 -----------------------*
	
	The goal of the algorithm is to subdivide the bounding rectangle (described as the union of the location of all the points)
	into a series of sub-rectangles, and interpolate the values of the known points to the new points found by the intersections
	of the lines:
	
	X-------*-----*---------*
	|       |     |         |
	|       |     |         |
	*-------X-----*---------*
	|       |     |         |
	|       |     |         |
	|       |     |         |
	|       |     |         |
	*-------*-----X---------*
	|       |     |         |
	|       |     |         |
	*-------*-----*---------X
	
	Above, X marks the original points, and the asterisks marks the points the algorithm inserted to create the sub-rectangles.
	For each asterisk above, the algorithm also interpolated the values from each of the given points (X's) to find the value
	at the new point (using bicubic interpolation.)
	
	Now that the algorithm has a list of rectangles, it can procede to render each of them using bicubic interpolation of the value
	across the surface.
	
	Note that for any CORNER points which are NOT given, they are assumed to have a value of "0". This can be changed in "testLine()", above.
*/

QList<qQuadValue> Interpolator::generateQuads(QList<qPointValue> points)
{
	m_pointsUsed.clear();
	
	// Sorted Y1->Y2 then X1->X2 (so for every Y, points go X1->X2, then next Y, etc, for example: (0,0), (1,0), (0,1), (1,1))
	//qSort(points.begin(), points.end(), qPointValue_sort_point);
	
	// Find the bounds of the point cloud given so we can then subdivide it into smaller rectangles as needed:
	QRectF bounds = getBounds(points);
	
	m_currentBounds = bounds; // used by pointKey(), isPointUsed(), markPointUsed()
	
	if(m_autoNearby)
		m_nearbyDistance = (bounds.width() * .25) * (bounds.height() * .25);
	
	// Iterate over every point
	// draw line from point to each bound (X1,Y),(X2,Y),(X,Y1),(X,Y2)
	// see if line intersects with perpendicular line from any other point
	// add point at intersection if none exists
	// add point at end point of line if none exists
	QList<qPointValue> outputList = points;
	
	if(points.size() < m_pointSizeCutoff)
	{
		qDebug() << "generateQuads(): Point method 1: Quad search";
		outputList << testLine(points, bounds.topLeft(),	bounds.topRight(),     1);
		outputList << testLine(points, bounds.topRight(),	bounds.bottomRight(),  2);
		outputList << testLine(points, bounds.bottomRight(),	bounds.bottomLeft(),   3);
		outputList << testLine(points, bounds.bottomLeft(),	bounds.topLeft(),      4);
	}
	else
	{
		qDebug() << "generateQuads(): Point method 2: Grid";
		float stepX = m_gridStepSizeX > 0 ? m_gridStepSizeX : bounds.width()  / m_gridNumStepsX;
		float stepY = m_gridStepSizeY > 0 ? m_gridStepSizeY : bounds.height() / m_gridNumStepsY;
// 		int  max = (int)(bounds.width() / stepX), count=0;
		for(float x=bounds.left(); x<=bounds.right()+stepX; x+=stepX)
		{
			//qDebug() << "generateQuads(): Processing column: "<<(count++)<<"/"<<max;
			for(float y=bounds.top(); y<=bounds.bottom()+stepY; y+=stepY)
			{
				QPointF point(x,y);
				double value = interpolateValue(point, points);
				//qDebug() << "generateQuads(): Processing point: "<<point<<", value: "<<value;
				outputList << qPointValue(point, value);
			}
		}
	}
	
	qDebug() << "generateQuads(): Final # points: "<<outputList.size();
	
	if(m_scaleValues)
	{
		double maxValue = 0;
		foreach(qPointValue p, outputList)
			if(p.value > maxValue)
				maxValue = p.value;
		
		QList<qPointValue> tmp;
		foreach(qPointValue p, outputList)
			tmp << qPointValue(p.point, p.value/maxValue);
		
		outputList = tmp;
	}
	
	//qDebug() << "generateQuads(): stepX:"<<stepX<<", stepY:"<<stepY<<", right:"<<bounds.right();
	
	/* After these four testLine() calls, the outputList now has these points (assuming the first example above):
	
	*-------X-----X---------X
	|       |     |         |
	|       |     |         |
	X-------*-----X---------X
	|       |     |         |
	|       |     |         |
	|       |     |         |
	|       |     |         |
	X-------X-----*---------X
	|       |     |         |
	|       |     |         |
	X-------X-----*---------*
	
	New points added by the four testLine() calls ar marked with an X.
	
	*/
	
	// Sort the list of ppoints so we procede thru the list top->bottom and left->right
	qSort(outputList.begin(), outputList.end(), _qPointValue_sort_point);
	
	// Points are now sorted Y1->Y2 then X1->X2 (so for every Y, points go X1->X2, then next Y, etc, for example: (0,0), (1,0), (0,1), (1,1))
	
	//qDebug() << "bounds:"<<bounds;

	//QList<QList<qPointValue> > quads;
	QList<qQuadValue> quads;
	
	// This is the last stage of the algorithm - go thru the new point cloud and construct the actual sub-rectangles
	// by starting with each point and proceding clockwise around the rectangle, getting the nearest point in each direction (X+,Y), (X,Y+) and (X-,Y)
	foreach(qPointValue tl, outputList)
	{
		QList<qPointValue> quad;
		
		qPointValue tr = nearestPoint(outputList, tl.point, 0);
		qPointValue br = nearestPoint(outputList, tr.point, 1);
		qPointValue bl = nearestPoint(outputList, br.point, 2);
		
		// These 'isNull()' tests catch points on the edges of the bounding rectangle
		if(!tr.point.isNull() &&
		   !br.point.isNull() &&
		   !bl.point.isNull())
		{
// 			quad  << tl << tr << br << bl;
// 			quads << (quad);
			quads << qQuadValue(tl, tr, br, bl);
			
			//qDebug() << "Quad[p]: "<<tl.point<<", "<<tr.point<<", "<<br.point<<", "<<bl.point;
// 			qDebug() << "Quad[v]: "<<tl.value<<tr.value<<br.value<<bl.value;
		}
	}
	
	return quads;
}

const QPointF operator*(const QPointF& a, const QPointF& b)
{
	return QPointF(a.x() * b.x(), a.y() * b.y());
}

const qPointValue operator*(const qPointValue& a, const QPointF& b)
{
	return qPointValue(a.point * b, a.value);
}

QImage Interpolator::renderPoints(QList<qPointValue> points, QSize renderSize, bool renderLines, bool renderPointValues)
{
	//renderLines = true;
	//renderPointValues = true;
	
	QRectF bounds = getBounds(points);
	
	QSizeF outputSize = bounds.size();
	double dx = 1, dy = 1;
	if(!renderSize.isNull())
	{
		outputSize.scale(renderSize, Qt::KeepAspectRatio);
		dx = outputSize.width()  / bounds.width();
		dy = outputSize.height() / bounds.height();
	}
	
	QPointF renderScale(dx,dy);
	
	QList<qQuadValue> quads = generateQuads(points);
	
	// Arbitrary rendering choices
//	QImage img(w,h, QImage::Format_ARGB32_Premultiplied);
	QImage img(outputSize.toSize(), QImage::Format_ARGB32_Premultiplied);
	
	QPainter p(&img);
	p.fillRect(img.rect(), Qt::white);
	
	int counter = 0, maxCounter = quads.size();
	
	//foreach(QList<qPointValue> quad, quads)
	foreach(qQuadValue quad, quads)
	{
 		qPointValue tl = quad.tl * renderScale; //quad[0];
 		qPointValue tr = quad.tr * renderScale; //quad[1];
		qPointValue br = quad.br * renderScale; //quad[2];
		qPointValue bl = quad.bl * renderScale; //quad[3];
		
		//qDebug() << "[quads]: pnt "<<(counter++)<<": "<<tl.point;
// 		int progress = (int)(((double)(counter++)/(double)maxCounter) * 100);
// 		qDebug() << "renderPoints(): Rendering rectangle, progress:"<<progress<<"%";
		
		int xmin = qMax((int)(tl.point.x()), 0);
		int xmax = qMin((int)(br.point.x()), (int)(img.width()));

		int ymin = qMax((int)(tl.point.y()), 0);
		int ymax = qMin((int)(br.point.y()), (int)(img.height()));

		// Here's the actual rendering of the interpolated quad
		for(int y=ymin; y<ymax; y++)
		{
			QRgb* scanline = (QRgb*)img.scanLine(y);
			const double sy = ((double)y) / dy;
			for(int x=xmin; x<xmax; x++)
			{
				double sx = ((double)x) / dx;
				double value = quadInterpolate(quad, sx, sy);
				//qDebug() << "Rendering quad "<<(counter++)<<"/"<<maxCounter<<": img point:"<<x<<","<<y<<", scaled:"<<sx<<","<<sy<<", value:"<<value;
				//QColor color = colorForValue(value);
				QColor color = isnan(value) ? Qt::gray : colorForValue(value);
				
				scanline[x] = color.rgba();
			}
		}
		
		if(renderLines)
		{
			QVector<QPointF> vec;
			vec << tl.point << tr.point << br.point << bl.point;
			p.setPen(QColor(0,0,0,127));
			p.drawPolygon(vec);
		}

		if(renderPointValues)
		{
			p.setPen(Qt::gray);
			qDrawTextOp(p,tl.point, QString().sprintf("%.02f",tl.value));
			qDrawTextOp(p,tr.point, QString().sprintf("%.02f",tr.value));
			qDrawTextOp(p,bl.point, QString().sprintf("%.02f",bl.value));
			qDrawTextOp(p,br.point, QString().sprintf("%.02f",br.value));
		}
	}

// 	p.setPen(QPen());
// 	p.setBrush(QColor(0,0,0,127));
// 	foreach(qPointValue v, points)
// 	{
// 		p.drawEllipse(v.point, 5, 5);
// 	}

// 	
// 	p.setPen(QPen());
// 	p.setBrush(QColor(255,255,255,200));
// 	counter = 0;
// 	foreach(qPointValue v, outputList)
// 	{
// 		p.setPen(QPen());
// 		if(!hasPoint(points, v.point))
// 			p.drawEllipse(v.point, 5, 5);
// 			
// // 		p.setPen(Qt::gray);
// // 		qDrawTextOp(p,v.point + QPointF(0, p.font().pointSizeF()*1.75), QString::number(counter++));
// 		
// 	}
	
	p.end();
	
	return img;
}

};

using namespace SurfaceInterpolate;

MainWindow::MainWindow()
{
	QRectF rect(0,0,4001,3420);
	//QRectF rect(0,0,300,300);
	//QRectF(89.856,539.641 1044.24x661.359)
	double w = rect.width(), h = rect.height();

	
	
	
	// Setup our list of points - these can be in any order, any arrangement.
	
// 	QList<qPointValue> points = QList<qPointValue>()
// 		<< qPointValue(QPointF(0,0), 		0.0)
// // 		<< qPointValue(QPointF(10,80), 		0.5)
// // 		<< qPointValue(QPointF(80,10), 		0.75)
// // 		<< qPointValue(QPointF(122,254), 	0.33)
// 		<< qPointValue(QPointF(w,0), 		0.0)
// 		<< qPointValue(QPointF(w/3*1,h/3*2),	1.0)
// 		<< qPointValue(QPointF(w/2,h/2),	1.0)
// 		<< qPointValue(QPointF(w/3*2,h/3*1),	1.0)
// 		<< qPointValue(QPointF(w,h), 		0.0)
// 		<< qPointValue(QPointF(0,h), 		0.0);

// 	QList<qPointValue> points = QList<qPointValue>()
// // 		<< qPointValue(QPointF(0,0), 		0.0)
// // 
// // 		<< qPointValue(QPointF(w*.5,h*.25),	1.0)
// // 		<< qPointValue(QPointF(w*.5,h*.75),	1.0)
// // 
// // 		<< qPointValue(QPointF(w*.5,h*.5),	0.5)
// // 
// // 		<< qPointValue(QPointF(w*.25,h*.5),	1.0)
// // 		<< qPointValue(QPointF(w*.75,h*.5),	1.0)
// // 
// // 		<< qPointValue(QPointF(w,w),		0.0);
// 		<< qPointValue(QPointF(0,0), 		0.0)
// 
// // 		<< qPointValue(QPointF(w*.2,h*.2),	1.0)
// // 		<< qPointValue(QPointF(w*.5,h*.2),	1.0)
// // 		<< qPointValue(QPointF(w*.8,h*.2),	1.0)
// 
// 		<< qPointValue(QPointF(w*.2,h*.8),	1.0)
// 		<< qPointValue(QPointF(w*.5,h*.8),	1.0)
// 		<< qPointValue(QPointF(w*.8,h*.8),	1.0)
// 
// 		<< qPointValue(QPointF(w*.8,h*.5),	1.0)
// 		<< qPointValue(QPointF(w*.8,h*.2),	1.0)
// 		
// 		//<< qPointValue(QPointF(w*.75,h),	1.0)
// 
// 		<< qPointValue(QPointF(w,w),		0.0)
// 		;

  

  	QList<qPointValue> points;// = QList<qPointValue>()
//   	points << qPointValue( QPointF(116.317, 2180.41), 0.6);
//         points << qPointValue( QPointF(408.87, 2939.64), 0.533333);
//         points << qPointValue( QPointF(71.2619, 3276.22), 0.52);
//         points << qPointValue( QPointF(326.109, 2682.47), 0.506667);
//         points << qPointValue( QPointF(246.38, 2017.56), 0.613333);
//         points << qPointValue( QPointF(210.603, 1790.57), 0.68);
//         points << qPointValue( QPointF(69.7606, 55.8084), 0.626667);
//         points << qPointValue( QPointF(1505.94, 59.3849), 0.68);
//         points << qPointValue( QPointF(2607.61, 53.2941), 0.693333);
//         points << qPointValue( QPointF(3934.88, 49.4874), 0.666667);
//         points << qPointValue( QPointF(3929.81, 1338.7), 0.693333);
//         points << qPointValue( QPointF(3942.07, 1899.13), 0.666667);
//         points << qPointValue( QPointF(2005.55, 1972.79), 0.906667);
//         points << qPointValue( QPointF(3811.29, 3186.99), 0.6);
// 		<< qPointValue(QPointF(0,0), 		0.0)
// 		<< qPointValue(QPointF(w,h),		0.0);

	
	points << qPointValue( QPointF(409.028, 2934.03), 0.286667);
        points << qPointValue( QPointF(263.632, 2944.56), 0.284444);
        points << qPointValue( QPointF(63.2585, 2967.1), 0.32);
        points << qPointValue( QPointF(65.64, 3271.95), 0.25);
        points << qPointValue( QPointF(168.789, 3272.29), 0.315556);
        points << qPointValue( QPointF(230.707, 3278.98), 0.306667);
        points << qPointValue( QPointF(58.8397, 3080.36), 0.213333);
        points << qPointValue( QPointF(196.52, 3077.11), 0.34);
        points << qPointValue( QPointF(302.373, 3041.57), 0.4);
        points << qPointValue( QPointF(264.275, 3032.89), 0.413333);
        points << qPointValue( QPointF(234.196, 2817.1), 0.28);
        points << qPointValue( QPointF(233.266, 2763.37), 0.426667);
        points << qPointValue( QPointF(226.289, 2715), 0.323333);
        points << qPointValue( QPointF(272.57, 2715.46), 0.435556);
        points << qPointValue( QPointF(315.827, 2697.09), 0.453333);
        points << qPointValue( QPointF(366.295, 2691.51), 0.36);
        points << qPointValue( QPointF(400.482, 2730.58), 0.326667);
        points << qPointValue( QPointF(485.137, 2724.07), 0.326667);
        points << qPointValue( QPointF(484.954, 2814.81), 0.27);
        points << qPointValue( QPointF(498.843, 2924.77), 0.42);
        points << qPointValue( QPointF(390.625, 2824.07), 0.364444);
        points << qPointValue( QPointF(306.134, 2627.31), 0.382222);
        points << qPointValue( QPointF(235.532, 2600.12), 0.408889);
        points << qPointValue( QPointF(307.827, 2570.06), 0.431111);
        points << qPointValue( QPointF(303.641, 2521.78), 0.462222);
        points << qPointValue( QPointF(344.108, 2555.27), 0.408889);
        points << qPointValue( QPointF(388.482, 2553.04), 0.513333);
        points << qPointValue( QPointF(394.621, 2580.95), 0.396667);
        points << qPointValue( QPointF(394.9, 2622.25), 0.417778);
        points << qPointValue( QPointF(487.835, 2466.52), 0.493333);
        points << qPointValue( QPointF(485.602, 2622.25), 0.391111);
        points << qPointValue( QPointF(490.067, 2542.15), 0.457778);
        points << qPointValue( QPointF(234.295, 2541.47), 0.408889);
        points << qPointValue( QPointF(231.883, 2494.05), 0.422222);
        points << qPointValue( QPointF(309.446, 2463.11), 0.422222);
        points << qPointValue( QPointF(235.902, 2433.77), 0.391111);
        points << qPointValue( QPointF(237.51, 2370.68), 0.39);
        points << qPointValue( QPointF(313.063, 2392.78), 0.363333);
        points << qPointValue( QPointF(161.555, 2335.71), 0.3);
        points << qPointValue( QPointF(101.675, 2316.82), 0.36);
        points << qPointValue( QPointF(128.199, 2250.92), 0.391111);
        points << qPointValue( QPointF(161.153, 2123.52), 0.377778);
        points << qPointValue( QPointF(69.9267, 2165.72), 0.326667);
        points << qPointValue( QPointF(195.312, 2208.72), 0.34);
        points << qPointValue( QPointF(198.527, 2312), 0.38);
        points << qPointValue( QPointF(268.454, 2286.68), 0.38);
        points << qPointValue( QPointF(304.623, 2189.83), 0.408889);
        points << qPointValue( QPointF(240.323, 2153.26), 0.368889);
        points << qPointValue( QPointF(282.52, 2110.66), 0.376667);
        points << qPointValue( QPointF(227.865, 2079.72), 0.373333);
        points << qPointValue( QPointF(308.642, 2050.38), 0.366667);
        points << qPointValue( QPointF(264.034, 2015.82), 0.393333);
        points << qPointValue( QPointF(184.06, 2040.73), 0.386667);
        points << qPointValue( QPointF(178.835, 1967.99), 0.346667);
        points << qPointValue( QPointF(179.237, 1884), 0.36);
        points << qPointValue( QPointF(172.807, 1846.23), 0.38);
        points << qPointValue( QPointF(116.946, 1854.26), 0.34);
        points << qPointValue( QPointF(111.32, 1929.82), 0.373333);
        points << qPointValue( QPointF(120.161, 1998.94), 0.391111);
        points << qPointValue( QPointF(73.9455, 2012.6), 0.376667);
        points << qPointValue( QPointF(63.4966, 2054.4), 0.38);
        points << qPointValue( QPointF(18.8882, 2049.17), 0.126667);
        points << qPointValue( QPointF(17.2807, 2092.17), 0.2);
        points << qPointValue( QPointF(70.3286, 1808.85), 0.435556);
        points << qPointValue( QPointF(137.04, 1799.21), 0.43);
        points << qPointValue( QPointF(75.1511, 1912.94), 0.42);
        points << qPointValue( QPointF(231.481, 1956.74), 0.41);
        points << qPointValue( QPointF(218.621, 1877.57), 0.431111);
        points << qPointValue( QPointF(217.416, 1802.02), 0.39);
        points << qPointValue( QPointF(216.21, 1703.56), 0.4);
        points << qPointValue( QPointF(73.1417, 1634.84), 0.382222);
        points << qPointValue( QPointF(67.9173, 1415.81), 0.373333);
        points << qPointValue( QPointF(63.0948, 1267.92), 0.373333);
        points << qPointValue( QPointF(107, 892), 0.368889);
        points << qPointValue( QPointF(116, 458), 0.364444);
        points << qPointValue( QPointF(127, 69), 0.32);
        points << qPointValue( QPointF(456, 73), 0.37);
        points << qPointValue( QPointF(668, 55), 0.37);
        points << qPointValue( QPointF(841.531, 58.8349), 0.312);
        points << qPointValue( QPointF(1225.73, 55.2582), 0.356667);
        points << qPointValue( QPointF(1500.62, 50.2347), 0.377778);
        points << qPointValue( QPointF(1617, 62.5143), 0.293333);
        points << qPointValue( QPointF(1572.9, 220.754), 0.333333);
        points << qPointValue( QPointF(1579.04, 361.132), 0.36);
        points << qPointValue( QPointF(1566.21, 594.165), 0.376667);
        points << qPointValue( QPointF(1615.88, 662.819), 0.393333);
        points << qPointValue( QPointF(1663.88, 597.514), 0.37);
        points << qPointValue( QPointF(1668.91, 485.323), 0.363333);
        points << qPointValue( QPointF(1662.21, 347.457), 0.36);
        points << qPointValue( QPointF(1650.77, 244.755), 0.346667);
        points << qPointValue( QPointF(1732.26, 253.406), 0.313333);
        points << qPointValue( QPointF(1918.41, 239.452), 0.33);
        points << qPointValue( QPointF(1922.87, 329.595), 0.35);
        points << qPointValue( QPointF(1941.85, 208.474), 0.306667);
        points << qPointValue( QPointF(1966.13, 149.03), 0.323333);
        points << qPointValue( QPointF(2005.2, 59.4444), 0.286667);
        points << qPointValue( QPointF(2094.79, 128.378), 0.346667);
        points << qPointValue( QPointF(2297.12, 138.424), 0.38);
        points << qPointValue( QPointF(2311.35, 51.351), 0.376667);
        points << qPointValue( QPointF(2397.31, 60.0025), 0.313333);
        points << qPointValue( QPointF(2496.66, 52.4673), 0.303333);
        points << qPointValue( QPointF(2570.62, 50.2347), 0.352);
        points << qPointValue( QPointF(2614.72, 4.46531), 0.234667);
        points << qPointValue( QPointF(2643.18, 51.351), 0.352);
        points << qPointValue( QPointF(2787.75, 61.677), 0.341333);
        points << qPointValue( QPointF(2870.35, 51.9092), 0.342222);
        points << qPointValue( QPointF(3031.94, 48.002), 0.386667);
        points << qPointValue( QPointF(3036.69, 106.051), 0.393333);
        points << qPointValue( QPointF(3093.9, 65.8633), 0.41);
        points << qPointValue( QPointF(3144.69, 117.214), 0.346667);
        points << qPointValue( QPointF(3317.16, 51.9092), 0.356667);
        points << qPointValue( QPointF(3474.84, 49.3974), 0.313333);
        points << qPointValue( QPointF(3558.29, 60.2816), 0.306667);
        points << qPointValue( QPointF(3553.55, 87.3525), 0.312);
        points << qPointValue( QPointF(3720.16, 43.8158), 0.31);
        points << qPointValue( QPointF(3876.72, 52.1883), 0.373333);
        points << qPointValue( QPointF(3939.24, 61.677), 0.311111);
        points << qPointValue( QPointF(3912.17, 101.586), 0.373333);
        points << qPointValue( QPointF(3945.38, 481.974), 0.37);
        points << qPointValue( QPointF(3947.61, 656.4), 0.417778);
        points << qPointValue( QPointF(3993.38, 651.935), 0.173333);
        points << qPointValue( QPointF(3984.73, 694.913), 0.173333);
        points << qPointValue( QPointF(3934, 1036), 0.346667);
        points << qPointValue( QPointF(3939.81, 1417.25), 0.36);
        points << qPointValue( QPointF(3927.66, 1576.97), 0.368889);
        points << qPointValue( QPointF(3931.71, 1733.22), 0.33);
        points << qPointValue( QPointF(3930.56, 1873.84), 0.34);
        points << qPointValue( QPointF(3568.87, 1925.93), 0.336667);
        points << qPointValue( QPointF(3180.56, 1917.25), 0.4);
        points << qPointValue( QPointF(2790.51, 1954.28), 0.413333);
        points << qPointValue( QPointF(2398.73, 1950.23), 0.433333);
        points << qPointValue( QPointF(2112.27, 1951.39), 0.45);
        points << qPointValue( QPointF(1979.85, 1968.22), 0.45);
        points << qPointValue( QPointF(1614.72, 1967.53), 0.422222);
        points << qPointValue( QPointF(1221.68, 1961.25), 0.408889);
        points << qPointValue( QPointF(833.291, 1964.04), 0.436667);
        points << qPointValue( QPointF(526.999, 1982.41), 0.42);
        points << qPointValue( QPointF(445.368, 1966.13), 0.4);
        points << qPointValue( QPointF(330.479, 1979.15), 0.4);
        points << qPointValue( QPointF(270.244, 1971.01), 0.39);
        points << qPointValue( QPointF(274.43, 1889.38), 0.4);
        points << qPointValue( QPointF(512.5, 1639.17), 0.36);
        points << qPointValue( QPointF(832.5, 1641.67), 0.41);
        points << qPointValue( QPointF(1230.83, 1629.17), 0.413333);
        points << qPointValue( QPointF(1494.17, 1640.83), 0.426667);
        points << qPointValue( QPointF(1611.53, 1649.04), 0.386667);
        points << qPointValue( QPointF(2042, 1636), 0.396667);
        points << qPointValue( QPointF(2396, 1633), 0.403333);
        points << qPointValue( QPointF(2794, 1635), 0.36);
        points << qPointValue( QPointF(3147.18, 1657.5), 0.343333);
        points << qPointValue( QPointF(3548.4, 1444.8), 0.377778);
        points << qPointValue( QPointF(3594, 1599.6), 0.336667);
        points << qPointValue( QPointF(3753.6, 1290), 0.36);
        points << qPointValue( QPointF(3544.56, 1358.02), 0.36);
        points << qPointValue( QPointF(3167.44, 1362.85), 0.373333);
        points << qPointValue( QPointF(2779.22, 1359.47), 0.383333);
        points << qPointValue( QPointF(2392.94, 1369.12), 0.396667);
        points << qPointValue( QPointF(2007.62, 1358.99), 0.4);
        points << qPointValue( QPointF(1612.17, 1359.47), 0.386667);
        points << qPointValue( QPointF(1228.3, 1360.92), 0.406667);
        points << qPointValue( QPointF(834.298, 1354.17), 0.408889);
        points << qPointValue( QPointF(736.883, 1288.1), 0.337778);
        points << qPointValue( QPointF(547.357, 1371.05), 0.435556);
        points << qPointValue( QPointF(465.229, 1337.92), 0.38);
        points << qPointValue( QPointF(363.922, 1298.85), 0.376667);
        points << qPointValue( QPointF(427.832, 904.224), 0.346667);
        points << qPointValue( QPointF(451.554, 703.844), 0.36);
        points << qPointValue( QPointF(831.663, 725.891), 0.37);
        points << qPointValue( QPointF(766.8, 339.6), 0.34);
        points << qPointValue( QPointF(1203.6, 757.2), 0.366667);
        points << qPointValue( QPointF(1615.88, 728.868), 0.333333);
        points << qPointValue( QPointF(2004.27, 726.542), 0.36);
        points << qPointValue( QPointF(2392.94, 733.796), 0.376667);
        points << qPointValue( QPointF(2775.46, 730.324), 0.364444);
        points << qPointValue( QPointF(3167.82, 729.167), 0.37);
        points << qPointValue( QPointF(846.354, 2953.32), 0.302222);
        points << qPointValue( QPointF(747.01, 2953.8), 0.243333);
        points << qPointValue( QPointF(604.745, 2959.59), 0.306667);
        points << qPointValue( QPointF(618.731, 3083.53), 0.28);
        points << qPointValue( QPointF(610.05, 3163.58), 0.24);
        points << qPointValue( QPointF(607.639, 3248.46), 0.266667);
        points << qPointValue( QPointF(666.782, 3267.26), 0.2);
        points << qPointValue( QPointF(727.063, 3270.95), 0.288889);
        points << qPointValue( QPointF(809.113, 3267.26), 0.293333);
        points << qPointValue( QPointF(861.023, 3269.61), 0.253333);
        points << qPointValue( QPointF(932.356, 3269.27), 0.284444);
        points << qPointValue( QPointF(810.788, 3316.49), 0.153333);
        points << qPointValue( QPointF(916.281, 3306.45), 0.186667);
        points << qPointValue( QPointF(857.004, 3330.22), 0.14);
        points << qPointValue( QPointF(926.997, 3164.45), 0.32);
        points << qPointValue( QPointF(929.342, 3080.39), 0.28);
        points << qPointValue( QPointF(989.958, 3051.25), 0.303333);
        points << qPointValue( QPointF(1098.47, 3038.53), 0.313333);
        points << qPointValue( QPointF(1197.93, 2997.34), 0.32);
        points << qPointValue( QPointF(1105.16, 2953.8), 0.3);
        points << qPointValue( QPointF(969.864, 2962.51), 0.326667);
        points << qPointValue( QPointF(892.838, 2951.12), 0.333333);
        points << qPointValue( QPointF(743.473, 3052.26), 0.333333);
        points << qPointValue( QPointF(745.483, 3153.06), 0.28);
        points << qPointValue( QPointF(865.644, 2876.64), 0.333333);
        points << qPointValue( QPointF(981.385, 2889.66), 0.313333);
        points << qPointValue( QPointF(1033.83, 2897.2), 0.313333);
        points << qPointValue( QPointF(1048.57, 2839.93), 0.284444);
        points << qPointValue( QPointF(1114.88, 2841.94), 0.326667);
        points << qPointValue( QPointF(1116.55, 2904.23), 0.326667);
        points << qPointValue( QPointF(1211.33, 2910.93), 0.368889);
        points << qPointValue( QPointF(1234.1, 2849.98), 0.383333);
        points << qPointValue( QPointF(1251.18, 2953.46), 0.333333);
        points << qPointValue( QPointF(1243.48, 3005.04), 0.336);
        points << qPointValue( QPointF(1251.18, 3069.67), 0.330667);
        points << qPointValue( QPointF(1253.86, 3228.08), 0.382222);
        points << qPointValue( QPointF(1233.76, 3275.64), 0.346667);
        points << qPointValue( QPointF(1321.84, 3220.71), 0.276667);
        points << qPointValue( QPointF(1472.21, 3218.37), 0.325333);
        points << qPointValue( QPointF(1638.66, 3214.35), 0.293333);
        points << qPointValue( QPointF(1760.89, 3265.59), 0.306667);
        points << qPointValue( QPointF(1826.87, 2998.68), 0.39);
        points << qPointValue( QPointF(1771, 2845), 0.26);
        points << qPointValue( QPointF(1763, 2616), 0.404444);
        points << qPointValue( QPointF(1769, 2412), 0.377778);
        points << qPointValue( QPointF(1794, 2232), 0.416667);
        points << qPointValue( QPointF(1825.19, 2169.8), 0.466667);
        points << qPointValue( QPointF(1726.87, 2078.91), 0.403333);
        points << qPointValue( QPointF(1616.75, 2087.35), 0.453333);
        points << qPointValue( QPointF(1279.31, 2080.05), 0.443333);
        points << qPointValue( QPointF(1219.36, 2026.13), 0.443333);
        points << qPointValue( QPointF(1041.2, 2026.8), 0.462222);
        points << qPointValue( QPointF(1032.49, 1969.53), 0.484444);
        points << qPointValue( QPointF(1037.18, 2397.87), 0.43);
        points << qPointValue( QPointF(1278.98, 2402.89), 0.453333);
        points << qPointValue( QPointF(1276.3, 2556.61), 0.36);
        points << qPointValue( QPointF(1279.31, 2669.14), 0.373333);
        points << qPointValue( QPointF(1281.32, 2748.84), 0.317333);
        points << qPointValue( QPointF(1286.68, 2818.17), 0.262222);
        points << qPointValue( QPointF(1286.01, 2882.13), 0.42);
        points << qPointValue( QPointF(1370, 2871.41), 0.35);
        points << qPointValue( QPointF(1596.96, 2826.72), 0.356667);
        points << qPointValue( QPointF(1606.71, 2655.2), 0.317333);
        points << qPointValue( QPointF(1562, 2417), 0.453333);
        points << qPointValue( QPointF(1458, 2217), 0.44);
        points << qPointValue( QPointF(1892.04, 2077.71), 0.408889);
        points << qPointValue( QPointF(1876.77, 2203.49), 0.423333);
        points << qPointValue( QPointF(1933.83, 2202.29), 0.391111);
        points << qPointValue( QPointF(1996.53, 2201.08), 0.413333);
        points << qPointValue( QPointF(2062.44, 2171.75), 0.48);
        points << qPointValue( QPointF(2131.96, 2170.14), 0.406667);
        points << qPointValue( QPointF(2056.01, 2091.37), 0.38);
        points << qPointValue( QPointF(1989.7, 2042.74), 0.366667);
        points << qPointValue( QPointF(1923.79, 2282.26), 0.343333);
        points << qPointValue( QPointF(1929.41, 2395.99), 0.34);
        points << qPointValue( QPointF(1935.83, 2633.33), 0.426667);
        points << qPointValue( QPointF(1914.17, 2839.17), 0.333333);
        points << qPointValue( QPointF(1955, 2987.5), 0.32);
        points << qPointValue( QPointF(1973.33, 3156.67), 0.314667);
        points << qPointValue( QPointF(2054.17, 3240.83), 0.258667);
        points << qPointValue( QPointF(2132.76, 3217.43), 0.284444);
        points << qPointValue( QPointF(2182.2, 3163.58), 0.346667);
        points << qPointValue( QPointF(2187.42, 3092.05), 0.315556);
        points << qPointValue( QPointF(2188.62, 3008.86), 0.303333);
        points << qPointValue( QPointF(2191.44, 2911.2), 0.326667);
        points << qPointValue( QPointF(2191.2, 2632.8), 0.368889);
        points << qPointValue( QPointF(2191.2, 2391.6), 0.313333);
        points << qPointValue( QPointF(2196.18, 2263.31), 0.306667);
        points << qPointValue( QPointF(2249.42, 2176.5), 0.366667);
        points << qPointValue( QPointF(2298.03, 2067.13), 0.396667);
        points << qPointValue( QPointF(2409.14, 2032.41), 0.4);
        points << qPointValue( QPointF(2391.2, 2159.72), 0.36);
        points << qPointValue( QPointF(2460.65, 2205.44), 0.34);
        points << qPointValue( QPointF(2539.35, 2164.93), 0.33);
        points << qPointValue( QPointF(2612.85, 2098.38), 0.36);
        points << qPointValue( QPointF(2458.91, 2276.62), 0.213333);
        points << qPointValue( QPointF(2455.44, 2424.19), 0.3);
        points << qPointValue( QPointF(2445.6, 2608.8), 0.355556);
        points << qPointValue( QPointF(2450.23, 2806.13), 0.333333);
        points << qPointValue( QPointF(2451.97, 3072.92), 0.276667);
        points << qPointValue( QPointF(2396.99, 3256.37), 0.34);
        points << qPointValue( QPointF(2581.6, 3228.59), 0.288889);
        points << qPointValue( QPointF(2676.5, 3248.84), 0.286667);
        points << qPointValue( QPointF(2653, 3032), 0.32);
        points << qPointValue( QPointF(2650, 2877), 0.346667);
        points << qPointValue( QPointF(2639, 2705), 0.28);
        points << qPointValue( QPointF(2631.37, 2647.57), 0.32);
        points << qPointValue( QPointF(2743.06, 2630.21), 0.28);
        points << qPointValue( QPointF(2743.06, 2482.06), 0.32);
        points << qPointValue( QPointF(2721.06, 2332.18), 0.346667);
        points << qPointValue( QPointF(2717.59, 2248.84), 0.366667);
        points << qPointValue( QPointF(2788.77, 2090.86), 0.346667);
        points << qPointValue( QPointF(2805.75, 2238.62), 0.34);
        points << qPointValue( QPointF(2965.37, 2181.23), 0.382222);
        points << qPointValue( QPointF(3091.72, 2185.09), 0.34);
        points << qPointValue( QPointF(3158.76, 2026.91), 0.404444);
        points << qPointValue( QPointF(3197.34, 2204.38), 0.36);
        points << qPointValue( QPointF(3187.21, 2341.82), 0.311111);
        points << qPointValue( QPointF(3207.95, 2640.82), 0.311111);
        points << qPointValue( QPointF(3205.54, 2822.14), 0.293333);
        points << qPointValue( QPointF(3187.69, 2920.52), 0.288889);
        points << qPointValue( QPointF(3193.48, 3084.01), 0.302222);
        points << qPointValue( QPointF(3165.51, 3257.14), 0.337778);
        points << qPointValue( QPointF(3230.76, 3218.37), 0.146667);
        points << qPointValue( QPointF(3260.23, 3227.08), 0.28);
        points << qPointValue( QPointF(3329.79, 3276.3), 0.302222);
        points << qPointValue( QPointF(3489.29, 3282.7), 0.236667);
        points << qPointValue( QPointF(3562.17, 3270.29), 0.275556);
        points << qPointValue( QPointF(3653.07, 3268.47), 0.36);
        points << qPointValue( QPointF(3659.09, 3200.55), 0.306667);
        points << qPointValue( QPointF(3813.82, 3258.02), 0.32);
        points << qPointValue( QPointF(3914.29, 3252.39), 0.2);
        points << qPointValue( QPointF(3914.29, 3134.64), 0.333333);
        points << qPointValue( QPointF(3838.73, 3016.89), 0.306667);
        points << qPointValue( QPointF(3662.31, 3028.55), 0.288889);
        points << qPointValue( QPointF(3567.87, 2936.52), 0.324444);
        points << qPointValue( QPointF(3652.26, 2803.1), 0.313333);
        points << qPointValue( QPointF(3678.39, 2705.44), 0.275556);
        points << qPointValue( QPointF(3938, 2609.39), 0.28);
        points << qPointValue( QPointF(3881.33, 2628.28), 0.328889);
        points << qPointValue( QPointF(3591.98, 2644.35), 0.353333);
        points << qPointValue( QPointF(3579.93, 2485.61), 0.303333);
        points << qPointValue( QPointF(3588.77, 2230.02), 0.342222);
        points << qPointValue( QPointF(3564.65, 2045.56), 0.271111);
        points << qPointValue( QPointF(3933.18, 2040.33), 0.343333);
        points << qPointValue( QPointF(3937.19, 2310.39), 0.271111);
        points << qPointValue( QPointF(3743.09, 2214.35), 0.355556);
        points << qPointValue( QPointF(3525.67, 2162.9), 0.333333);
        points << qPointValue( QPointF(3421.99, 2166.52), 0.328889);
        points << qPointValue( QPointF(3329.56, 2195.86), 0.346667);
        points << qPointValue( QPointF(3266.06, 2219.97), 0.373333);
        points << qPointValue( QPointF(3354.87, 2056.41), 0.343333);
        points << qPointValue( QPointF(985.806, 2453.46), 0.466667);
        points << qPointValue( QPointF(982.189, 2506.91), 0.431111);
        points << qPointValue( QPointF(980.179, 2618.63), 0.391111);
        points << qPointValue( QPointF(976.964, 2742.41), 0.404444);
        points << qPointValue( QPointF(979.376, 2806.71), 0.364444);
        points << qPointValue( QPointF(875.289, 2580.86), 0.44);
        points << qPointValue( QPointF(884.131, 2494.05), 0.457778);
        points << qPointValue( QPointF(761.96, 2488.43), 0.426667);
        points << qPointValue( QPointF(625.723, 2492.85), 0.488889);
        points << qPointValue( QPointF(547.759, 2499.68), 0.433333);
        points << qPointValue( QPointF(539.32, 2390.37), 0.43);
        points << qPointValue( QPointF(539.32, 2264.58), 0.413333);
        points << qPointValue( QPointF(534.095, 2152.05), 0.426667);
        points << qPointValue( QPointF(536.506, 2054.8), 0.422222);
        points << qPointValue( QPointF(757, 2129), 0.476667);
        points << qPointValue( QPointF(750, 2292), 0.444444);
        points << qPointValue( QPointF(905, 2269), 0.466667);
        points << qPointValue( QPointF(729.861, 2863.19), 0.366667);
        points << qPointValue( QPointF(661.111, 2880.56), 0.346667);
        points << qPointValue( QPointF(656.25, 2910.42), 0.337778);
        points << qPointValue( QPointF(706.635, 2891.84), 0.293333);
        points << qPointValue( QPointF(591.095, 2885.48), 0.353333);
        points << qPointValue( QPointF(538.181, 2887.49), 0.453333);
        points << qPointValue( QPointF(537.176, 2965.86), 0.306667);
        points << qPointValue( QPointF(540.19, 3005.37), 0.364444);
        points << qPointValue( QPointF(534.162, 3053.6), 0.32);
        points << qPointValue( QPointF(497.993, 3051.25), 0.355556);
        points << qPointValue( QPointF(448.428, 3055.27), 0.324444);
        points << qPointValue( QPointF(397.524, 3055.61), 0.328889);
        points << qPointValue( QPointF(355.327, 3051.25), 0.355556);
        points << qPointValue( QPointF(329.874, 3007.38), 0.296667);
        points << qPointValue( QPointF(279.975, 2985.61), 0.368889);
        points << qPointValue( QPointF(324.516, 2940.07), 0.342222);
        points << qPointValue( QPointF(358.341, 2893.18), 0.351111);
        points << qPointValue( QPointF(325.856, 2828.88), 0.306667);
        points << qPointValue( QPointF(267.249, 2873.76), 0.355556);
        points << qPointValue( QPointF(306.097, 2750.85), 0.4);
        points << qPointValue( QPointF(441.061, 2672.49), 0.48);
        points << qPointValue( QPointF(532.153, 2677.84), 0.44);
        points << qPointValue( QPointF(537.846, 2612.2), 0.52);
        points << qPointValue( QPointF(542.2, 2542.54), 0.52);
        points << qPointValue( QPointF(539.186, 2750.85), 0.413333);
        points << qPointValue( QPointF(540.86, 2805.1), 0.373333);
        points << qPointValue( QPointF(235.098, 3052.26), 0.311111);
        points << qPointValue( QPointF(190.892, 3126.27), 0.284444);
        points << qPointValue( QPointF(154.723, 3172.82), 0.23);
        points << qPointValue( QPointF(67.9843, 3151.05), 0.208889);
        points << qPointValue( QPointF(116.879, 3212.68), 0.3);
        points << qPointValue( QPointF(221.033, 3182.53), 0.236667);
        points << qPointValue( QPointF(126.257, 3085.41), 0.303333);
        points << qPointValue( QPointF(132.62, 3020.78), 0.355556);
        points << qPointValue( QPointF(57.6024, 3019.44), 0.29);
        points << qPointValue( QPointF(75.352, 2896.87), 0.36);
        points << qPointValue( QPointF(128.601, 2899.21), 0.306667);
        points << qPointValue( QPointF(211.655, 2954.13), 0.373333);
        points << qPointValue( QPointF(219.358, 2864.38), 0.386667);
        points << qPointValue( QPointF(135.634, 2835.92), 0.377778);
        points << qPointValue( QPointF(68.989, 2833.57), 0.406667);
        points << qPointValue( QPointF(69.9937, 2766.93), 0.302222);
        points << qPointValue( QPointF(79.7057, 2631.63), 0.426667);
        points << qPointValue( QPointF(80.0406, 2662.44), 0.395556);
        points << qPointValue( QPointF(129.605, 2733.77), 0.406667);
        points << qPointValue( QPointF(185.868, 2688.23), 0.328889);
        points << qPointValue( QPointF(181.515, 2623.93), 0.313333);
        points << qPointValue( QPointF(70.6635, 2564.98), 0.4);
        points << qPointValue( QPointF(124.917, 2515.08), 0.4);
        points << qPointValue( QPointF(150.034, 2559.29), 0.364444);
        points << qPointValue( QPointF(413.194, 2096.64), 0.37);
        points << qPointValue( QPointF(419.56, 2382.52), 0.426667);
        


	Interpolator ip;
	
	QList<qPointValue> originalInputs = points;

	// Set bounds to be at minimum the bounds of the background
	points << qPointValue( QPointF(0,0), ip.interpolateValue(QPointF(0,0), originalInputs ) );
	QPointF br = QPointF((double)w, (double)h);
	points << qPointValue( br, ip.interpolateValue(br, originalInputs) );
	
// 	srand(time(NULL));

// 	for(int i=0; i<100; i++)
// 	{
// 		points << qPointValue(QPointF((double)rand()/(double)RAND_MAX*w, (double)rand()/(double)RAND_MAX*h), (double)rand()/(double)RAND_MAX);
// 		qDebug() << "points << qPointValue("<<points.last().point<<", "<<points.last().value<<");";
// 	}
	
// 	points << qPointValue( QPointF(77.8459, 259.802), 0.980021);
// 	points << qPointValue( QPointF(220.788, 23.7931), 0.445183);

// 	points << qPointValue( QPointF(136.454, 214.625), 0.684312);
// 	points << qPointValue( QPointF(2.11327, 169.876), 0.669569);
// 	points << qPointValue( QPointF(150.927, 244.475), 0.25839);
// 	points << qPointValue( QPointF(289.744, 107.131), 0.764456);
// 	points << qPointValue( QPointF(8.46319, 294.867), 0.996068);
//  



// points << qPointValue( QPointF(145.152, 881.28), 0.8);
// points << qPointValue( QPointF(89.856, 603.072), 0.183333);
// points << qPointValue( QPointF(361.152, 1007.42), 0.216667);
// points << qPointValue( QPointF(1134.1, 936.776), 0.1);
// points << qPointValue( QPointF(884.8, 789.019), 0.0166667);
// points << qPointValue( QPointF(832.851, 660.204), 0.183333);
// points << qPointValue( QPointF(944.251, 539.641), 0.05);
// points << qPointValue( QPointF(793.522, 979.344), 0.0833333);
// points << qPointValue( QPointF(746.287, 1065.78), 0.266667);
// points << qPointValue( QPointF(777.358, 1191.72), 0.1);
// points << qPointValue( QPointF(1057, 1201), 0.0166667);
// points << qPointValue( QPointF(975, 1043), 0.1);
// 
// 





	// Fuzzing, above, produced these points, which aren't rendred properly:
// 	points << qPointValue(QPointF(234.93, 118.315),  0.840188);
// 	qDebug() << "Added point: "<<points.last().point<<", val:"<<points.last().value;
// 	points << qPointValue(QPointF(59.2654, 273.494), 0.79844);
// 	qDebug() << "Added point: "<<points.last().point<<", val:"<<points.last().value;
	

	//exit(-1);


//  	#define val(x) ( ((double)x) / 6. )
//  	QList<qPointValue> points = QList<qPointValue>()
//  		<< qPointValue(QPointF(0,0),	val(0))
//  		
// 		<< qPointValue(QPointF(w/4*1,h/4*1),	val(1))
// 		<< qPointValue(QPointF(w/4*2,h/4*1),	val(2))
// 		<< qPointValue(QPointF(w/4*3,h/4*1),	val(4))
// 		<< qPointValue(QPointF(w/4*4,h/4*1),	val(1))
// 		
// 		<< qPointValue(QPointF(w/4*1,h/4*2),	val(6))
// 		<< qPointValue(QPointF(w/4*2,h/4*2),	val(3))
// 		<< qPointValue(QPointF(w/4*3,h/4*2),	val(5))
// 		<< qPointValue(QPointF(w/4*4,h/4*2),	val(2))
// 		
// 		<< qPointValue(QPointF(w/4*1,h/4*3),	val(4))
// 		<< qPointValue(QPointF(w/4*2,h/4*3),	val(2))
// 		<< qPointValue(QPointF(w/4*3,h/4*3),	val(1))
// 		<< qPointValue(QPointF(w/4*4,h/4*3),	val(5))
// 		
// 		<< qPointValue(QPointF(w/4*1,h/4*4),	val(5))
// 		<< qPointValue(QPointF(w/4*2,h/4*4),	val(4))
// 		<< qPointValue(QPointF(w/4*3,h/4*4),	val(2))
// 		<< qPointValue(QPointF(w/4*4,h/4*4),	val(3))
// 		
// 		;
// 
// 	#undef val
	
	
	QImage img = ip.renderPoints(points, QSize(500,500));
	
	//exit(-1);
	/*
	
	// Just some debugging:
	
	QImage img(4,4, QImage::Format_ARGB32_Premultiplied);
	
	QList<QList<qPointValue> > quads;
	
	QList<qPointValue> points;
	
// 	points = QList<qPointValue>()
// 		<< qPointValue(QPointF(0,0), 					0.0)
// 		<< qPointValue(QPointF(img.width()/2,0),			0.5)
// 		<< qPointValue(QPointF(img.width()/2,img.height()), 		0.0)
// 		<< qPointValue(QPointF(0,img.height()), 			0.5);
// 	quads << points;
// 	
// 	points = QList<qPointValue>()
// 		<< qPointValue(QPointF(img.width()/2,0), 		0.5)
// 		<< qPointValue(QPointF(img.width(),0),			0.0)
// 		<< qPointValue(QPointF(img.width(),img.height()), 	0.5)
// 		<< qPointValue(QPointF(img.width()/2,img.height()), 	0.0);
// 	quads << points;


	points = QList<qPointValue>()
		<< qPointValue(QPointF(0,0), 				0.0)
		<< qPointValue(QPointF(img.width()-1,0),		0.0)
		<< qPointValue(QPointF(img.width()-1,img.height()-1), 	1.0)
		<< qPointValue(QPointF(0,img.height()-1), 		0.0);
	quads << points;
	
	
	//quads << (points);
	foreach(QList<qPointValue> corners, quads)
	{
		//for(int y=0; y<img.height(); y++)
		for(int y=corners[0].point.y(); y<=corners[2].point.y(); y++)
		{
			QRgb* scanline = (QRgb*)img.scanLine(y);
			for(int x=corners[0].point.x(); x<=corners[2].point.x(); x++)
			{
				double value = quadInterpolate(corners, (double)x, (double)y);
				qDebug() << "pnt:"<<x<<","<<y<<", val:"<<value;
				QColor color = colorForValue(value);
				scanline[x] = color.rgba();
			}
		}
	}
	*/
	//img = img.scaled(rect.size().toSize());
	img.save("interpolate.jpg");

	setPixmap(QPixmap::fromImage(img));//.scaled(200,200)));
	
	exit(-1);
}


