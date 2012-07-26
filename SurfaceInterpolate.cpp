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


#define fuzzyIsEqual(a,b) ( fabs(a - b) < 0.00001 )
//#define fuzzyIsEqual(a,b) ( ((int) a) == ((int) b) )

namespace SurfaceInterpolate {

/// \brief Simple value->color function
QColor Interpolator::colorForValue(double v)
{
	//int hue = qMax(0, qMin(359, (int)((1-v) * 359)));
	//return QColor::fromHsv(hue, 255, 255);
	
	int hue = (int)((1-v) * (359-120) + 120);
 	hue = qMax(0, qMin(359, hue));

// 	int hue = (int)(v * 120);
// 	hue = qMax(0, qMin(120, hue));
 	 
	int sat = 255;
	int val = 255;
	double valTop = 1.0;//0.9;
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
double quadInterpolate(qQuadValue quad, double x, double y, bool useBicubic = true)
{
	double w = quad.width();
	double h = quad.height();
	
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

		double h1  = (quad.br.point.y() - y),
		       h2  = (y - quad.tl.point.y());
		
		double p1  = h1 / h * fr1
			   + h2 / h * fr2;
	
		//qDebug() << "quadInterpolate: "<<x<<" x "<<y<<": "<<p1<<" (h1:"<<h1<<", h2:"<<h2<<", fr1:"<<fr1<<",fr2:"<<fr2<<",w:"<<w<<",h:"<<h<<"), bry:"<<quad.br.point.y()<<", tly:"<<quad.tl.point.y();
		
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
	double p = 1.5;
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
	
//	int count=0, max=inputs.size();
	foreach(qPointValue v, inputs)
	{
		//qDebug() << "testLine(): dir:"<<dir<<", processing point:"<<(count++)<<"/"<<max;
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
				//qDebug() << "testLine: "<<boundLine<<" -> "<<line<<" ["<<dir<<"/1] New point: "<<point<<", value:"<<value;//<<", corners: "<<c1.value<<","<<c2.value;
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
			if(fuzzyIsEqual(point2.y(), 0))
				point2.setY(0);
			if(fuzzyIsEqual(point2.x(), 0))
				point2.setX(0);

			if(!point2.isNull())
			{
				if(!isPointUsed(point2))
				{
					markPointUsed(point2);
					double value = interpolateValue(point2, inputs);
					outputs << qPointValue(point2, isnan(value) ? 0:value);
// 					qDebug() << "testLine: "<<boundLine<<" -> "<<line<<" -> "<<line2<<" ["<<dir<<"/2]";
// 					qDebug() << "\t New point: "<<point2<<", value:"<<value;//<<", corners: "<<c1.value<<","<<c2.value;
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

		if(v.point == point)
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

QList<qQuadValue> Interpolator::generateQuads(QList<qPointValue> points, bool forceGrid)
{
	m_pointsUsed.clear();
	
	// Sorted Y1->Y2 then X1->X2 (so for every Y, points go X1->X2, then next Y, etc, for example: (0,0), (1,0), (0,1), (1,1))
	//qSort(points.begin(), points.end(), qPointValue_sort_point);
	
	// Find the bounds of the point cloud given so we can then subdivide it into smaller rectangles as needed:
	QRectF bounds = getBounds(points);
	
	m_currentBounds = bounds; // used by pointKey(), isPointUsed(), markPointUsed()

	foreach(qPointValue p, points)
		markPointUsed(p.point);
	
	if(m_autoNearby)
		m_nearbyDistance = (bounds.width() * .35) * (bounds.height() * .35);
	
	// Iterate over every point
	// draw line from point to each bound (X1,Y),(X2,Y),(X,Y1),(X,Y2)
	// see if line intersects with perpendicular line from any other point
	// add point at intersection if none exists
	// add point at end point of line if none exists
	QList<qPointValue> outputList = points;
	
	if(!forceGrid &&
	    points.size() < m_pointSizeCutoff)
	{
		qDebug() << "# generateQuads(): Point method 1: Quad search";
		outputList << testLine(points, bounds.topLeft(),	bounds.topRight(),     1);
		outputList << testLine(points, bounds.topRight(),	bounds.bottomRight(),  2);
		outputList << testLine(points, bounds.bottomRight(),	bounds.bottomLeft(),   3);
		outputList << testLine(points, bounds.bottomLeft(),	bounds.topLeft(),      4);
	}
	else
	{
		qDebug() << "# generateQuads(): Point method 2: Grid";
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
	
	qDebug() << "# generateQuads(): Final # points: "<<outputList.size();
	
	#if 1
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
	#endif
	
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
// 	int counter=0;
/*	foreach(qPointValue tl, outputList)
		qDebug() << "point:"<<tl.point;*/
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
// 			counter++;
// 			if(counter == 7)
// 			{
//				qDebug() << "Quad[p]: "<<tl.point<<", "<<tr.point<<", "<<br.point<<", "<<bl.point;
//				qDebug() << "Quad[v]: "<<tl.value<<", "<<tr.value<<", "<<br.value<<", "<<bl.value;
//			}
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
// 	renderLines = true;
// 	renderPointValues = true;
	
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
	
	//int counter = 0, maxCounter = quads.size();

	//qDebug() << "renderPoints(): renderScale: "<<renderScale;
	
	//foreach(QList<qPointValue> quad, quads)
	foreach(qQuadValue quad, quads)
	{
 		qPointValue tl = quad.tl * renderScale; //quad[0];
 		qPointValue tr = quad.tr * renderScale; //quad[1];
		qPointValue br = quad.br * renderScale; //quad[2];
		qPointValue bl = quad.bl * renderScale; //quad[3];
		
		//qDebug() << "[quads]: pnt "<<(counter++)<<": "<<tl.point;
//		int progress = (int)(((double)(counter++)/(double)maxCounter) * 100);

		int xmin = qMax((int)(tl.point.x()), 0);
		int xmax = qMin((int)(br.point.x()+1), (int)(img.width()));

		int ymin = qMax((int)(tl.point.y()), 0);
		int ymax = qMin((int)(br.point.y()+1), (int)(img.height()));

// 		if(counter == 7)
// 		{
// 			qDebug() << "renderPoints(): Rendering rectangle, progress:"<<progress<<"%: counter:"<<counter<<": "<<quad;
// 			qDebug() << "renderPoints(): Rendering rectangle: "<<quad;
// 			qDebug() << "\tymin:"<<ymin<<", ymax:"<<ymax<<", xmin:"<<xmin<<", xmax:"<<xmax;
// 		}
		

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

	
// 	p.setPen(QPen());
// 	p.setBrush(QColor(255,255,255,200));
// //	counter = 0;
// 	foreach(qPointValue v, points)
// 	{
// 		p.setPen(QPen());
// 		//if(!hasPoint(points, v.point))
// 			p.drawEllipse(v.point * renderScale, 5, 5);
// 			qDrawTextOp(p, (v.point * renderScale + QPointF(0, p.font().pointSizeF()*1.2)), QString().sprintf("%.02f",v.value));
// 
// // 		p.setPen(Qt::gray);
// // 		qDrawTextOp(p,v.point + QPointF(0, p.font().pointSizeF()*1.75), QString::number(counter++));
// 
// 	}
	
	p.end();
	
	return img;
}

bool Interpolator::write3dSurfaceFile(QString objFile, QList<qPointValue> points, QSizeF renderSize, double valueScale, QString mtlFile, double xres, double yres)
{
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

// 	// TODO - Provide separate step X/Y for 3D grid?
// 	int oldStepX = m_gridNumStepsX;
// 	int oldStepY = m_gridNumStepsY;
// 
// 	m_gridNumStepsX = 100;
// 	m_gridNumStepsY = 100;

	// TBD - Is this a good formula?
	if(valueScale < 0)
		valueScale = sqrt(outputSize.width() * outputSize.height()) * 0.25;
	
	QList<qQuadValue> quads = generateQuads(points, true); // force grid mode

// 	m_gridNumStepsX = oldStepX;
// 	m_gridNumStepsY = oldStepY;

	QStringList bufferVerts;
	QStringList bufferFaces;
	QStringList bufferMaterial;
	
	QStringList materialsUsed;

	qDebug() << "# Using valueScale: "<<valueScale;
	
	foreach(qQuadValue quad, quads)
	{
 		qPointValue tl = quad.tl * renderScale;
 		qPointValue tr = quad.tr * renderScale;
		qPointValue br = quad.br * renderScale;
		qPointValue bl = quad.bl * renderScale;

		/*
		int startIdx = bufferVerts.size();
		bufferVerts << QString("v %1 %2 %3").arg(tl.point.x()).arg(tl.value * valueScale).arg(tl.point.y());
		bufferVerts << QString("v %1 %2 %3").arg(tr.point.x()).arg(tr.value * valueScale).arg(tr.point.y());
		bufferVerts << QString("v %1 %2 %3").arg(br.point.x()).arg(br.value * valueScale).arg(br.point.y());
		bufferVerts << QString("v %1 %2 %3").arg(bl.point.x()).arg(bl.value * valueScale).arg(bl.point.y());

		int idx0 = startIdx + 1; // vert indexes are 1 based
		int idx1 = idx0 + 1;
		int idx2 = idx1 + 1;
		int idx3 = idx2 + 1;
		bufferFaces << QString("f %1 %2 %3 %4").arg(idx0).arg(idx1).arg(idx2).arg(idx3);
		*/
		
		double xmin = tl.point.x();
		double xmax = br.point.x();

		double ymin = tl.point.y();
		double ymax = br.point.y();

		double yStep = (ymax - ymin) / yres;
		double xStep = (xmax - xmin) / xres;

		qQuadValue quad2(tl, tr, br, bl);
				
		#define WriteVert(x,y) \
			bufferVerts << QString("v %1 %2 %3") \
				.arg(x) \
				.arg(quadInterpolate(quad2, x, y)) \
				.arg(y);

		for(double y=ymin; y<ymax; y+=yStep)
		{
			for(double x=xmin; x<xmax; x+=xStep)
			{
				int startIdx = bufferVerts.size();

				WriteVert(x,         y);
				WriteVert(x + xStep, y);
				WriteVert(x + xStep, y + yStep);
				WriteVert(x,         y + yStep);

				// get color at center
				double value = quadInterpolate(quad2, x+xStep/2, y+yStep/2);
				QColor color = isnan(value) ? Qt::gray : colorForValue(value);

				QString mtlName = color.name();
				mtlName = mtlName.replace("#", "mtl");

				if(!materialsUsed.contains(mtlName))
				{
					materialsUsed << mtlName;
					bufferMaterial << QString("newmtl %1\nKa %2 %3 %4\nKd %2 %3 %4\n")
						.arg(mtlName)
						.arg(color.redF())
						.arg(color.greenF())
						.arg(color.blueF());
				}

				bufferFaces << QString("usemtl %1").arg(mtlName);

				int idx0 = startIdx + 1; // vert indexes are 1 based
				int idx1 = idx0 + 1;
				int idx2 = idx1 + 1;
				int idx3 = idx2 + 1;
				bufferFaces << QString("f %1 %2 %3 %4").arg(idx0).arg(idx1).arg(idx2).arg(idx3);
			}
		}
	}

	qDebug() << "generate3dSurface(): Num verts: "<<bufferVerts.size()<<", Num faces: "<<bufferFaces.size();

	if(mtlFile.isEmpty())
		mtlFile = QFileInfo(objFile).baseName() + ".mtl";

	QString objCode = QString("mtllib %1\n%2\n%3\n")
		.arg(mtlFile)
		.arg(bufferVerts.join("\n"))
		.arg(bufferFaces.join("\n"));

	QString mtlCode = bufferMaterial.join("\n") + "\n";

	QFile file(objFile);
	if(!file.open(QIODevice::WriteOnly))
	{
		return false;
	}
	else
	{
		QTextStream stream(&file);
		stream << objCode;
		file.close();
	}

	QFile file2(mtlFile);
	if(!file2.open(QIODevice::WriteOnly))
	{
		return false;
	}
	else
	{
		QTextStream stream(&file2);
		stream << mtlCode;
		file2.close();
	}

	return true;
		
}

};
