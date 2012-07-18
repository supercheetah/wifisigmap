#include <QtGui>

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

QColor colorForValue(double v)
{
	return QColor::fromHsv((int)((1-v) * 359), 255, 255);
}

class qPointValue {
public:
	qPointValue(QPointF p = QPointF()) : point(p), value(0.0), valueIsNull(true) {}
	qPointValue(QPointF p, double v)   : point(p), value(v),   valueIsNull(false) {}
	
	QPointF point;
	double  value;
	bool    valueIsNull;
	
	void setValue(double v)
	{
		value       = v;
		valueIsNull = false;
	}
	
	bool isNull()  { return  point.isNull() ||  valueIsNull; }
	bool isValid() { return !point.isNull() && !valueIsNull; }
};

// The following cubic interpolation functions are from http://www.paulinternet.nl/?page=bicubic
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

double tricubicInterpolate (double p[4][4][4], double x, double y, double z) {
	double arr[4];
	arr[0] = bicubicInterpolate(p[0], y, z);
	arr[1] = bicubicInterpolate(p[1], y, z);
	arr[2] = bicubicInterpolate(p[2], y, z);
	arr[3] = bicubicInterpolate(p[3], y, z);
	return cubicInterpolate(arr, x);
}

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

bool newQuad = false;
double quadInterpolate(QList<qPointValue> corners, double x, double y)
{
	double w = corners[2].point.x() - corners[0].point.x();
	double h = corners[2].point.y() - corners[0].point.y();
	
	bool useBicubic = true;
	if(useBicubic)
	{
		// Use bicubic impl from http://www.paulinternet.nl/?page=bicubic
		double unitX = (x - corners[0].point.x()) / w;
		double unitY = (y - corners[0].point.y()) / h;
		
		double	c1 = corners[0].value,
			c2 = corners[1].value,
			c3 = corners[2].value,
			c4 = corners[3].value;

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
			
			newQuad = false;
		//}
		
		return p2;
	}
	else
	{
		// Very simple implementation of http://en.wikipedia.org/wiki/Bilinear_interpolation
		double fr1 = (corners[2].point.x() - x) / w * corners[0].value
			   + (x - corners[0].point.x()) / w * corners[1].value;
	
		double fr2 = (corners[2].point.x() - x) / w * corners[3].value
			   + (x - corners[0].point.x()) / w * corners[2].value;
	
		double p1  = (corners[2].point.y() - y) / h * fr1
			   + (y - corners[0].point.y()) / h * fr2;
	
		//qDebug() << "quadInterpolate: "<<x<<" x "<<y<<": "<<p<<" (fr1:"<<fr1<<",fr2:"<<fr2<<",w:"<<w<<",h:"<<h<<")";
		return p1;
	}
}

bool qPointValue_sort_point(qPointValue a, qPointValue b)
{
	return a.point.y() == b.point.y() ?
		a.point.x() < b.point.x() :
		a.point.y() < b.point.y();
}

bool hasPoint(QList<qPointValue> list, QPointF point)
{
	foreach(qPointValue v, list)
		if(v.point == point)
			return true;
	return false;
}

qPointValue getPoint(QList<qPointValue> list, QPointF point)
{
	foreach(qPointValue v, list)
		if(v.point == point)
			return v;
	return qPointValue();
}

QList<qPointValue> testLine(QList<qPointValue> inputs, QList<qPointValue> existingPoints, QRectF bounds, QPointF p1, QPointF p2, int dir)
{
	QLineF boundLine(p1,p2);
	QList<qPointValue> outputs;
	
	qPointValue c1, c2;
	if(!hasPoint(existingPoints, boundLine.p1()) &&
	   !hasPoint(outputs,        boundLine.p1()))
	{
		c1 = qPointValue(boundLine.p1(), 0.); // NOTE assumingunknown corners have value of 0.
		outputs << c1;
	}
	else
	{
		c1 = getPoint(inputs, boundLine.p1());
	}
	
	if(!hasPoint(existingPoints, boundLine.p2()) &&
	   !hasPoint(outputs,        boundLine.p2()))
	{
		c2 = qPointValue(boundLine.p2(), 0.); // NOTE assuming unknown corners have value of 0.;
		outputs << c2;
	}
	else
	{
		c2 = getPoint(inputs, boundLine.p2());
	}
		
	double pval[4] = { c1.value, c1.value/2, c2.value/2, c2.value };
	 
	double lineLength = boundLine.length();
	
	foreach(qPointValue v, inputs)
	{
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
			if(!hasPoint(existingPoints, point) &&
			   !hasPoint(outputs, point))
			{
				double unitVal = (dir == 1 || dir == 3 ? v.point.x() : v.point.y()) / lineLength;
				double value = cubicInterpolate(pval, unitVal);
				outputs << qPointValue(point, value);
				//qDebug() << "testLine: "<<boundLine<<" -> "<<line<<" ["<<dir<<"/1] New point: "<<point<<", value:"<<value<<", corners: "<<c1.value<<","<<c2.value;
			}
		}
		
	}
	
	
	return outputs;
}


QList<qPointValue> testInnerLines(QList<qPointValue> inputs, QList<qPointValue> existingPoints, QRectF bounds, QPointF p1, QPointF p2, int dir)
{
	QLineF boundLine(p1,p2);
	QList<qPointValue> outputs;
	
// 	qPointValue c1, c2;
// 	if(!hasPoint(existingPoints, boundLine.p1()) &&
// 	   !hasPoint(outputs,        boundLine.p1()))
// 	{
// 		c1 = qPointValue(boundLine.p1(), 0.); // NOTE assumingunknown corners have value of 0.
// 		outputs << c1;
// 	}
// 	else
// 	{
// 		c1 = getPoint(inputs, boundLine.p1());
// 	}
// 	
// 	if(!hasPoint(existingPoints, boundLine.p2()) &&
// 	   !hasPoint(outputs,        boundLine.p2()))
// 	{
// 		c2 = qPointValue(boundLine.p2(), 0.); // NOTE assuming unknown corners have value of 0.;
// 		outputs << c2;
// 	}
// 	else
// 	{
// 		c2 = getPoint(inputs, boundLine.p2());
// 	}
// 		
// 	double pval[4] = { c1.value, c1.value/2, c2.value/2, c2.value };
// 	 
// 	double lineLength = boundLine.length();
	
	foreach(qPointValue v, inputs)
	{
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
		
// 		// test to see boundLine intersects line, if so, create point if none exists
// 		QPointF point;
// 		/*QLineF::IntersectType type = */boundLine.intersect(line, &point);
// 		if(!point.isNull())
// 		{
// 			if(!hasPoint(existingPoints, point) &&
// 			   !hasPoint(outputs, point))
// 			{
// 				double unitVal = (dir == 1 || dir == 3 ? v.point.x() : v.point.y()) / lineLength;
// 				double value = cubicInterpolate(pval, unitVal);
// 				outputs << qPointValue(point, value);
// 				qDebug() << "testLine: "<<boundLine<<" -> "<<line<<" ["<<dir<<"/1] New point: "<<point<<", value:"<<value<<", corners: "<<c1.value<<","<<c2.value;
// 			}
// 		}
		

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
				if(!hasPoint(existingPoints, point2) &&
				   !hasPoint(outputs, point2))
				{
					// Get values for the ends of the line (assuming interpolated above)
					qPointValue ic1 = getPoint(existingPoints, line2.p1());
					if(ic1.isNull())
						ic1 = getPoint(outputs, line2.p1());
					
					qPointValue ic2 = getPoint(existingPoints, line2.p2());
					if(ic2.isNull())
						ic2 = getPoint(outputs, line2.p2());
						
					if(ic1.isNull())
					{
						qDebug() << "ic1 logic error, quiting";
						exit(-1);
					}
					
					if(ic2.isNull())
					{
						qDebug() << "ic2 logic error, quiting";
						exit(-1);
					}
					
					double innerPval[4] = { ic1.value, ic1.value/2, ic2.value/2, ic2.value };
			
					double innerLineLength = line2.length();
			
					
					
					// Note we are using perpendicular point components to the cubicInterpolate() call above
					double unitVal = (dir == 1 || dir == 3 ? v.point.y() : v.point.x()) / innerLineLength;
					double value = cubicInterpolate(innerPval, unitVal);
					outputs << qPointValue(point2, value);
					qDebug() << "testLine: "<<boundLine<<" -> "<<line<<" ["<<dir<<"/2] New point: "<<point2<<", value:"<<value<<", corners: "<<ic1.value<<","<<ic2.value;
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
		if(((dir == 0 && v.point.y() == point.y() && val > origin) ||
		    (dir == 1 && v.point.x() == point.x() && val > origin) ||
		    (dir == 2 && v.point.y() == point.y() && val < origin) ||
		    (dir == 3 && v.point.x() == point.x() && val < origin) )  &&
		    dist < minDist)
		{
			minPnt = v;
			minDist = dist;
		}
	}
	
	return minPnt;
}

MainWindow::MainWindow()
{
	
	QRectF rect(0,0,300,300);
	double w = rect.width(), h = rect.height();

	// Rectangle Bicubic Interpolation
	
	// The goal:
	// Given an arbitrary list of points (and values assoicated with each point),
	// interpolate the value for every point contained within the bounding rectangle.
	// Goal something like: http://en.wikipedia.org/wiki/File:BicubicInterpolationExample.png
	
	/*
	
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

	QList<qPointValue> points = QList<qPointValue>()
		<< qPointValue(QPointF(0,0), 		0.0)
		//<< qPointValue(QPointF(w/3*2,h/3),	1.0)
		<< qPointValue(QPointF(w/4*3,h/4),	1.0)
		<< qPointValue(QPointF(w/4,h/4*2),	1.0)
		<< qPointValue(QPointF(w,w),		0.0);

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
	
	
		
	

	// Sorted Y1->Y2 then X1->X2 (so for every Y, points go X1->X2, then next Y, etc, for example: (0,0), (1,0), (0,1), (1,1))
	//qSort(points.begin(), points.end(), qPointValue_sort_point);
	
	// Find the bounds of the point cloud given so we can then subdivide it into smaller rectangles as needed:
	QRectF bounds;
	foreach(qPointValue v, points)
	{
		//qDebug() << "corner: "<<v.point<<", value:"<<v.value;
		bounds |= QRectF(v.point, QSizeF(1.,1.0));
	}
	bounds.adjust(0,0,-1,-1);
	
	// Iterate over every point
	// draw line from point to each bound (X1,Y),(X2,Y),(X,Y1),(X,Y2)
	// see if line intersects with perpendicular line from any other point
	// add point at intersection if none exists
	// add point at end point of line if none exists
	QList<qPointValue> outputList = points;
	
	outputList << testLine(points, outputList, bounds, bounds.topLeft(),		bounds.topRight(),     1);
	outputList << testLine(points, outputList, bounds, bounds.topRight(),		bounds.bottomRight(),  2);
	outputList << testLine(points, outputList, bounds, bounds.bottomRight(),	bounds.bottomLeft(),   3);
	outputList << testLine(points, outputList, bounds, bounds.bottomLeft(),		bounds.topLeft(),      4);
	
	/* After these four testLine() calls, the outputList now has these points (assuming the first example above):
	
	*-------X-----X---------X
	|       |     |         |
	|       |     |         |
	X-------*-----?---------X
	|       |     |         |
	|       |     |         |
	|       |     |         |
	|       |     |         |
	X-------?-----*---------X
	|       |     |         |
	|       |     |         |
	X-------X-----*---------*
	
	New points added by the four testLine() calls ar marked with an X.
	
	Note the two question marks above where the inner lines intersect - those points were NOT added because
	their values could not be interpolated until the values for the outer points were calculated.
	 
	Now that every outer point along the outside edges of the outer rectangle have been calculated, we
	now can go thru and calculate the intersection of every line inside the rectangle.
	
	*/
		
	// 
	outputList << testInnerLines(points, outputList, bounds, bounds.topLeft(),	bounds.topRight(),     1);
	outputList << testInnerLines(points, outputList, bounds, bounds.topRight(),	bounds.bottomRight(),  2);
	outputList << testInnerLines(points, outputList, bounds, bounds.bottomRight(),	bounds.bottomLeft(),   3);
	outputList << testInnerLines(points, outputList, bounds, bounds.bottomLeft(),	bounds.topLeft(),      4);
	
	
	// Sort the list of ppoints so we procede thru the list top->bottom and left->right
	qSort(outputList.begin(), outputList.end(), qPointValue_sort_point);
	
	// Points are now sorted Y1->Y2 then X1->X2 (so for every Y, points go X1->X2, then next Y, etc, for example: (0,0), (1,0), (0,1), (1,1))
	
	
	//qDebug() << "bounds:"<<bounds;

	QList<QList<qPointValue> > quads;
	
	// Arbitrary rendering choices
//	QImage img(w,h, QImage::Format_ARGB32_Premultiplied);
	QImage img(400,400, QImage::Format_ARGB32_Premultiplied);
	
	QPainter p(&img);
	// Move painter 50,50 so we have room for text (when debugging I rendered values over every point)
	p.fillRect(img.rect(), Qt::white);
//	int topX=0, topY=0;
 	int topX=50, topY=50;
  	p.translate(topX,topY);

// 	
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
			quad  << tl << tr << br << bl;
			quads << (quad);
			
// 			qDebug() << "Quad[p]: "<<tl.point<<", "<<tr.point<<", "<<br.point<<", "<<bl.point;
// 			qDebug() << "Quad[v]: "<<tl.value<<tr.value<<br.value<<bl.value;
		
			// Here's the actual rendering of the interpolated quad
			for(int y=(int)tl.point.y(); y<br.point.y(); y++)
			{
				QRgb* scanline = (QRgb*)img.scanLine(y+topY);
				for(int x=(int)tl.point.x(); x<br.point.x(); x++)
				{
					double value = quadInterpolate(quad, (double)x, (double)y);
					QColor color = colorForValue(value);
					scanline[x+topX] = color.rgba();
				}
			}
			
			QVector<QPointF> vec;
			vec <<tl.point<<tr.point<<br.point<<bl.point;
			p.setPen(QColor(0,0,0,127));
			p.drawPolygon(vec);

			p.setPen(Qt::gray);
			p.drawText(tl.point, QString().sprintf("%.02f",tl.value));
			p.drawText(tr.point, QString().sprintf("%.02f",tr.value));
			p.drawText(bl.point, QString().sprintf("%.02f",bl.value));
			p.drawText(br.point, QString().sprintf("%.02f",br.value));
		}
	}
	
	p.end();
	
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
	img.save("interpolate.jpg");

	setPixmap(QPixmap::fromImage(img));//.scaled(200,200)));
}


