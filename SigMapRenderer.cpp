#include "SigMapRenderer.h"
#include "MapGraphicsScene.h"


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

static QString SigMapRenderer_sort_apMac;
static QPointF SigMapRenderer_sort_center;

bool SigMapRenderer_sort_SigMapValue(SigMapValue *a, SigMapValue *b)
{
	QString apMac  = SigMapRenderer_sort_apMac;
	QPointF center = SigMapRenderer_sort_center;

// 	double va = a && a->hasAp(apMac) ? a->signalForAp(apMac) : 0.;
// 	double vb = b && b->hasAp(apMac) ? b->signalForAp(apMac) : 0.;
// 	return va < vb;

	double aDist = 0.0;
	double bDist = 0.0;

	// sqrt() commented out because its not needed just for sorting (e.g. a squared value compares the same as a sqrt'ed value)
	if(a)
	{
		if(a->renderDataDirty)
		{
			QPointF calcPoint = a->point - center;
			aDist = /*sqrt*/(calcPoint.x() * calcPoint.x() + calcPoint.y() * calcPoint.y());

			a->renderLevel     = aDist;
			a->renderDataDirty = false;
		}
		else
		{
			aDist = a->renderLevel;
		}
	}

	if(b)
	{
		if(b->renderDataDirty)
		{
			QPointF calcPoint = b->point - center;
			bDist = /*sqrt*/(calcPoint.x() * calcPoint.x() + calcPoint.y() * calcPoint.y());

			b->renderLevel     = bDist;
			b->renderDataDirty = false;
		}
		else
		{
			bDist = b->renderLevel;
		}
	}

	return aDist > bDist;
}


/// fillTriColor() is a routine I translated from Delphi, atrribution below.

class TRGBFloat {
public:
	float R;
	float G;
	float B;
};

bool fillTriColor(QImage *img, QVector<QPointF> points, QList<QColor> colors)
{
	if(!img)
		return false;

	if(img->format() != QImage::Format_ARGB32_Premultiplied &&
	   img->format() != QImage::Format_ARGB32)
		return false;

	double imgWidth  = img->width();
	double imgHeight = img->height();

	/// The following routine translated from Delphi, originally found at:
	// http://www.swissdelphicenter.ch/en/showcode.php?id=1780

	double  LX, RX, Ldx, Rdx, Dif1, Dif2;

	TRGBFloat LRGB, RRGB, RGB, RGBdx, LRGBdy, RRGBdy;

	QColor RGBT;
	double y, x, ScanStart, ScanEnd;// : integer;
	bool Right;
	QPointF tmpPoint;
	QColor tmpColor;


	 // sort vertices by Y
	int Vmax = 0;
	if (points[1].y() > points[0].y())
		Vmax = 1;
	if (points[2].y() > points[Vmax].y())
		 Vmax = 2;

	if(Vmax != 2)
	{
		tmpPoint = points[2];
		points[2] = points[Vmax];
		points[Vmax] = tmpPoint;

		tmpColor = colors[2];
		colors[2] = colors[Vmax];
		colors[Vmax] = tmpColor;
	}

	if(points[1].y() > points[0].y())
		Vmax = 1;
	else
		Vmax = 0;

	if(Vmax == 0)
	{
		tmpPoint = points[1];
		points[1] = points[0];
		points[0] = tmpPoint;

		tmpColor = colors[1];
		colors[1] = colors[0];
		colors[0] = tmpColor;
	}

	Dif1 = points[2].y() - points[0].y();
	if(Dif1 == 0)
		Dif1 = 0.001; // prevent div by 0 error

	Dif2 = points[1].y() - points[0].y();
	if(Dif2 == 0)
		Dif2 = 0.001;

	//work out if middle point is to the left or right of the line
	// connecting upper and lower points
	if(points[1].x() > (points[2].x() - points[0].x()) * Dif2 / Dif1 + points[0].x())
		Right = true;
	else	Right = false;

	   // calculate increments in x and colour for stepping through the lines
	if(Right)
	{
		Ldx = (points[2].x() - points[0].x()) / Dif1;
		Rdx = (points[1].x() - points[0].x()) / Dif2;
		LRGBdy.B = (colors[2].blue()  - colors[0].blue())  / Dif1;
		LRGBdy.G = (colors[2].green() - colors[0].green()) / Dif1;
		LRGBdy.R = (colors[2].red()   - colors[0].red())   / Dif1;
		RRGBdy.B = (colors[1].blue()  - colors[0].blue())  / Dif2;
		RRGBdy.G = (colors[1].green() - colors[0].green()) / Dif2;
		RRGBdy.R = (colors[1].red()   - colors[0].red())   / Dif2;
	}
	else
	{
		Ldx = (points[1].x() - points[0].x()) / Dif2;
		Rdx = (points[2].x() - points[0].x()) / Dif1;

		RRGBdy.B = (colors[2].blue()  - colors[0].blue())  / Dif1;
		RRGBdy.G = (colors[2].green() - colors[0].green()) / Dif1;
		RRGBdy.R = (colors[2].red()   - colors[0].red())   / Dif1;
		LRGBdy.B = (colors[1].blue()  - colors[0].blue())  / Dif2;
		LRGBdy.G = (colors[1].green() - colors[0].green()) / Dif2;
		LRGBdy.R = (colors[1].red()   - colors[0].red())   / Dif2;
	}

	LRGB.R = colors[0].red();
	LRGB.G = colors[0].green();
	LRGB.B = colors[0].blue();
	RRGB = LRGB;

	LX = points[0].x() - 1; /* -1: fix for not being able to render in floating-point coorindates */
	RX = points[0].x();

	// fill region 1
	for(y = points[0].y(); y <= points[1].y() - 1; y++)
	{
		// y clipping
		if(y > imgHeight - 1)
			break;

		if(y < 0)
		{
			LX = LX + Ldx;
			RX = RX + Rdx;
			LRGB.B = LRGB.B + LRGBdy.B;
			LRGB.G = LRGB.G + LRGBdy.G;
			LRGB.R = LRGB.R + LRGBdy.R;
			RRGB.B = RRGB.B + RRGBdy.B;
			RRGB.G = RRGB.G + RRGBdy.G;
			RRGB.R = RRGB.R + RRGBdy.R;
			continue;
		}

		// Scan = ABitmap.ScanLine[y];

		// calculate increments in color for stepping through pixels
		Dif1 = RX - LX + 1;
		if(Dif1 == 0)
			Dif1 = 0.001;
		RGBdx.B = (RRGB.B - LRGB.B) / Dif1;
		RGBdx.G = (RRGB.G - LRGB.G) / Dif1;
		RGBdx.R = (RRGB.R - LRGB.R) / Dif1;

		// x clipping
		if(LX < 0)
		{
			ScanStart = 0;
			RGB.B = LRGB.B + (RGBdx.B * fabs(LX));
			RGB.G = LRGB.G + (RGBdx.G * fabs(LX));
			RGB.R = LRGB.R + (RGBdx.R * fabs(LX));
		}
		else
		{
			RGB = LRGB;
			ScanStart = LX; //round(LX);
		}

		// Was RX-1 - inverted to fix not being able to render in floatingpoint coords
		if(RX + 1 > imgWidth - 1)
			ScanEnd = imgWidth - 1;
		else	ScanEnd = RX + 1; //round(RX) - 1;


		// scan the line
		QRgb* scanline = (QRgb*)img->scanLine((int)y);
		for(x = ScanStart; x <= ScanEnd; x++)
		{
			scanline[(int)x] = qRgb((int)RGB.R, (int)RGB.G, (int)RGB.B);

			RGB.B = RGB.B + RGBdx.B;
			RGB.G = RGB.G + RGBdx.G;
			RGB.R = RGB.R + RGBdx.R;
		}

		// increment edge x positions
		LX = LX + Ldx;
		RX = RX + Rdx;

		// increment edge colours by the y colour increments
		LRGB.B = LRGB.B + LRGBdy.B;
		LRGB.G = LRGB.G + LRGBdy.G;
		LRGB.R = LRGB.R + LRGBdy.R;
		RRGB.B = RRGB.B + RRGBdy.B;
		RRGB.G = RRGB.G + RRGBdy.G;
		RRGB.R = RRGB.R + RRGBdy.R;
	}


	Dif1 = points[2].y() - points[1].y();
	if(Dif1 == 0)
		Dif1 = 0.001;

	// calculate new increments for region 2
	if(Right)
	{
		Rdx = (points[2].x() - points[1].x()) / Dif1;
		RX  = points[1].x();
		RRGBdy.B = (colors[2].blue()  - colors[1].blue())  / Dif1;
		RRGBdy.G = (colors[2].green() - colors[1].green()) / Dif1;
		RRGBdy.R = (colors[2].red()   - colors[1].red())   / Dif1;
		RRGB.R = colors[1].red();
		RRGB.G = colors[1].green();
		RRGB.B = colors[1].blue();
	}
	else
	{
		Ldx = (points[2].x() - points[1].x()) / Dif1;
		LX  = points[1].x();
		LRGBdy.B = (colors[2].blue()  - colors[1].blue())  / Dif1;
		LRGBdy.G = (colors[2].green() - colors[1].green()) / Dif1;
		LRGBdy.R = (colors[2].red()   - colors[1].red())   / Dif1;
		LRGB.R = colors[1].red();
		LRGB.G = colors[1].green();
		LRGB.B = colors[1].blue();
	}

	// fill region 2
	for(y = points[1].y(); y < points[2].y() - 1; y++)
	{
		// y clipping
		if(y > imgHeight - 1)
			break;
		if(y < 0)
		{
			LX = LX + Ldx;
			RX = RX + Rdx;
			LRGB.B = LRGB.B + LRGBdy.B;
			LRGB.G = LRGB.G + LRGBdy.G;
			LRGB.R = LRGB.R + LRGBdy.R;
			RRGB.B = RRGB.B + RRGBdy.B;
			RRGB.G = RRGB.G + RRGBdy.G;
			RRGB.R = RRGB.R + RRGBdy.R;
			continue;
		}

		//Scan := ABitmap.ScanLine[y];

		Dif1 = RX - LX + 1;
		if(Dif1 == 0)
			Dif1 = 0.001;

		RGBdx.B = (RRGB.B - LRGB.B) / Dif1;
		RGBdx.G = (RRGB.G - LRGB.G) / Dif1;
		RGBdx.R = (RRGB.R - LRGB.R) / Dif1;

		// x clipping
		if(LX < 0)
		{
			ScanStart = 0;
			RGB.B = LRGB.B + (RGBdx.B * fabs(LX));
			RGB.G = LRGB.G + (RGBdx.G * fabs(LX));
			RGB.R = LRGB.R + (RGBdx.R * fabs(LX));
		}
		else
		{
			RGB = LRGB;
			ScanStart = LX; //round(LX);
		}

		// Was RX-1 - inverted to fix not being able to render in floatingpoint coords
		if(RX + 1 > imgWidth - 1)
			ScanEnd = imgWidth - 1;
		else 	ScanEnd = RX + 1; //round(RX) - 1;

		// scan the line
		QRgb* scanline = (QRgb*)img->scanLine((int)y);
		for(x = ScanStart; x < ScanEnd; x++)
		{
			scanline[(int)x] = qRgb((int)RGB.R, (int)RGB.G, (int)RGB.B);

			RGB.B = RGB.B + RGBdx.B;
			RGB.G = RGB.G + RGBdx.G;
			RGB.R = RGB.R + RGBdx.R;
		}

		LX = LX + Ldx;
		RX = RX + Rdx;

		LRGB.B = LRGB.B + LRGBdy.B;
		LRGB.G = LRGB.G + LRGBdy.G;
		LRGB.R = LRGB.R + LRGBdy.R;
		RRGB.B = RRGB.B + RRGBdy.B;
		RRGB.G = RRGB.G + RRGBdy.G;
		RRGB.R = RRGB.R + RRGBdy.R;
	}

	return true;
}


SigMapRenderer::SigMapRenderer(MapGraphicsScene* gs)
	// Cannot set gs as QObject() parent because then we can't move it to a QThread
	: QObject(), m_gs(gs)
{
	qRegisterMetaType<QPicture>("QPicture");
}


#define fuzzyIsEqual(a,b) ( fabs(a - b) < 0.00001 )
//#define fuzzyIsEqual(a,b) ( ((int) a) == ((int) b) )

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

class qQuadValue : public QRectF {
public:
	qQuadValue(
		qPointValue _tl,
		qPointValue _tr,
		qPointValue _br,
		qPointValue _bl)
		
		: QRectF(_tl.point.x(),  _tl.point.y(),
		         _tr.point.x() - _tl.point.x(),
		         _br.point.y() - _tr.point.y())
		
		, tl(_tl)
		, tr(_tr)
		, br(_br)
		, bl(_bl)
	{}
	
	qQuadValue(QList<qPointValue> corners)
	
		: QRectF(corners[0].point.x(),  corners[0].point.y(),
		         corners[1].point.x() - corners[0].point.x(),
		         corners[2].point.y() - corners[1].point.y())
		
		, tl(corners[0])
		, tr(corners[1])
		, br(corners[2])
		, bl(corners[3])
	{}
		
	QList<qPointValue> corners()
	{
		return QList<qPointValue>()
			<< tl << tr << br << bl;
	};
	
	qPointValue tl, tr, br, bl;
};

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

/// \brief Calculates the weightof point \a o at point \a i given power \a p, used internally by interpolateValue()
double weight(QPointF i, QPointF o, double p)
{
	return 1/pow(QLineF(i,o).length(), p);
}

/// \brief A very simple IDW implementation to interpolate the value at \a point from the inverse weighted distance of all the other points given in \a inputs, using a default power factor 
double interpolateValue(QPointF point, QList<qPointValue> inputs)
{
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

QList<qPointValue> testLine(QList<qPointValue> inputs, QList<qPointValue> existingPoints, QRectF bounds, QPointF p1, QPointF p2, int dir)
{
	QLineF boundLine(p1,p2);
	QList<qPointValue> outputs;
	
	if(!hasPoint(existingPoints, boundLine.p1()) &&
	   !hasPoint(outputs,        boundLine.p1()))
	{
		double value = interpolateValue(boundLine.p1(), inputs);
		outputs << qPointValue(boundLine.p1(), value);
	}
	
	if(!hasPoint(existingPoints, boundLine.p2()) &&
	   !hasPoint(outputs,        boundLine.p2()))
	{
		double value = interpolateValue(boundLine.p2(), inputs);
		outputs << qPointValue(boundLine.p2(), value);;
	}
	
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
// 				double unitVal = (dir == 1 || dir == 3 ? v.point.x() : v.point.y()) / lineLength;
// 				double value = cubicInterpolate(pval, unitVal);
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
				if(!hasPoint(existingPoints, point2) &&
				   !hasPoint(outputs, point2))
				{
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

QList<qQuadValue> generateQuads(QList<qPointValue> points)
{
	// Sorted Y1->Y2 then X1->X2 (so for every Y, points go X1->X2, then next Y, etc, for example: (0,0), (1,0), (0,1), (1,1))
	//qSort(points.begin(), points.end(), qPointValue_sort_point);
	
	// Find the bounds of the point cloud given so we can then subdivide it into smaller rectangles as needed:
	QRectF bounds = getBounds(points);
	
	// Iterate over every point
	// draw line from point to each bound (X1,Y),(X2,Y),(X,Y1),(X,Y2)
	// see if line intersects with perpendicular line from any other point
	// add point at intersection if none exists
	// add point at end point of line if none exists
	QList<qPointValue> outputList = points;
	
	qDebug() << "generateQuads(): "<<points.size()<<" points: processing line top:    left->right";
	outputList << testLine(points, outputList, bounds, bounds.topLeft(),		bounds.topRight(),     1);
	qDebug() << "generateQuads(): "<<points.size()<<" points: processing line right:  top->bottom";
	outputList << testLine(points, outputList, bounds, bounds.topRight(),		bounds.bottomRight(),  2);
	qDebug() << "generateQuads(): "<<points.size()<<" points: processing line bottom: right->left";
	outputList << testLine(points, outputList, bounds, bounds.bottomRight(),	bounds.bottomLeft(),   3);
	qDebug() << "generateQuads(): "<<points.size()<<" points: processing line left:   bottom->top";
	outputList << testLine(points, outputList, bounds, bounds.bottomLeft(),		bounds.topLeft(),      4);
	
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
	
	qDebug() << "generateQuads(): Final point cloud size: "<<outputList.size()<<" points, sorting..."; 
	
	// Sort the list of ppoints so we procede thru the list top->bottom and left->right
	qSort(outputList.begin(), outputList.end(), _qPointValue_sort_point);
	
	// Points are now sorted Y1->Y2 then X1->X2 (so for every Y, points go X1->X2, then next Y, etc, for example: (0,0), (1,0), (0,1), (1,1))
	
	//qDebug() << "bounds:"<<bounds;

	//QList<QList<qPointValue> > quads;
	QList<qQuadValue> quads;
	
	qDebug() << "generateQuads(): Generating quads from "<<outputList.size()<<" points in cloud.";
	
	int counter = 0, max = outputList.size();
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
			
			qDebug() << "generateQuads(): Point "<<(counter++)<<"/"<<max<<": New quad generated, num quads now: "<<quads.size();
			
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

QImage renderPoints(QList<qPointValue> points, QSize renderSize = QSize(), bool renderLines=false, bool renderPointValues=false)
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
	
	qDebug() << "renderPoints(): Generating quads for "<<points.size()<<" points, bounds: "<<bounds<<", output size:"<<outputSize;
	QList<qQuadValue> quads = generateQuads(points);
	qDebug() << "renderPoints(): Generated "<<quads.size()<<" from "<<points.size()<<" points";
	
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
		int progress = (int)(((double)(counter++)/(double)maxCounter) * 100);
		qDebug() << "renderPoints(): Rendering rectangle, progress:"<<progress<<"%";
		
		int xmin = qMax((int)(tl.point.x()), 0);
		int xmax = qMin((int)(br.point.x()), (int)(img.width()));

		int ymin = qMax((int)(tl.point.y()), 0);
		int ymax = qMin((int)(br.point.y()), (int)(img.height()));


		// Here's the actual rendering of the interpolated quad
		for(int y=ymin; y<ymax; y++)
		{
			QRgb* scanline = (QRgb*)img.scanLine(y);
			for(int x=xmin; x<xmax; x++)
			{
				double value = quadInterpolate(quad, ((double)x) / dx, ((double)y) / dy);
				//QColor color = colorForValue(value);
				QColor color = colorForValue(value);
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
// 
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

void SigMapRenderer::render()
{
	//qDebug() << "SigMapRenderer::render(): currentThreadId:"<<QThread::currentThreadId()<<", rendering...";
	QSize origSize = m_gs->m_bgPixmap.size();

#ifdef Q_OS_ANDROID
	if(origSize.isEmpty())
		origSize = QApplication::desktop()->screenGeometry().size();
#endif

// 	QSize renderSize = origSize;
// 	// Scale size down 1/3rd just so the renderTriangle() doesn't have to fill as many pixels
// 	renderSize.scale((int)(origSize.width() * .33), (int)(origSize.height() * .33), Qt::KeepAspectRatio);
//
// 	double dx = ((double)renderSize.width())  / ((double)origSize.width());
// 	double dy = ((double)renderSize.height()) / ((double)origSize.height());
//
// 	QImage mapImg(renderSize, QImage::Format_ARGB32_Premultiplied);
	double dx=1., dy=1.;
	/*
	QImage mapImg(origSize, QImage::Format_ARGB32_Premultiplied);
	QPainter p(&mapImg);*/

 	QPicture pic;
 	QPainter p(&pic);
	p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
	//p.fillRect(mapImg.rect(), Qt::transparent);

	QHash<QString,QString> apMacToEssid;

	foreach(SigMapValue *val, m_gs->m_sigValues)
		foreach(WifiDataResult result, val->scanResults)
			if(!apMacToEssid.contains(result.mac))
				apMacToEssid.insert(result.mac, result.essid);

	//qDebug() << "SigMapRenderer::render(): Unique MACs: "<<apMacToEssid.keys()<<", mac->essid: "<<apMacToEssid;

	int numAps = apMacToEssid.keys().size();
	int idx = 0;

	(void)numAps; // avoid unused warnings

	if(m_gs->m_renderMode == MapGraphicsScene::RenderRectangles)
	{
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



		QStringList macList;
		foreach(MapApInfo *info, m_gs->m_apInfo)
			if(info->renderOnMap)
				macList << info->mac;


		//qDebug() << "MAC: "<<apMac;
		QList<qPointValue> points;
		//MapApInfo *info = m_gs->apInfo(apMac);

		//QPointF center = info->point;

// 		int macListSize = macList.size();
		foreach(SigMapValue *val, m_gs->m_sigValues)
		{
			double sum = 0;
			int count  = 0;
			foreach(QString apMac, macList)
			{
				if(val->hasAp(apMac))
				{
					sum   += val->signalForAp(apMac);
					count ++;
				}
			}

			qPointValue p = qPointValue(val->point,  count == 0 ? 0 : sum/(double)count); // signalForAp() returns 0 if not in this reading
			points << p;
			qDebug() << "\tpoints << qPointValue(" << p.point << ", " << p.value << ");";
		}

		QList<qPointValue> originalInputs = points;

		// Set bounds to be at minimum the bounds of the background
		points << qPointValue( QPointF(0,0), interpolateValue(QPointF(0,0), originalInputs ) );
		if(!origSize.isEmpty())
		{
			QPointF br = QPointF((double)origSize.width(), (double)origSize.height());
			points << qPointValue( br, interpolateValue(br, originalInputs) );
		}

// 		srand(time(NULL));
// 		for(int i=0; i<50; i++)
// 		{
// 			QPointF randomPoint(QPointF((double)rand()/(double)RAND_MAX*origSize.width(), (double)rand()/(double)RAND_MAX* origSize.height()));
// 			points << qPointValue(randomPoint, interpolateValue(randomPoint, originalInputs) * 0.9);
// 		}

		QImage tmpImg = renderPoints(points, QSize(1000,1000));
		
		tmpImg = tmpImg.scaled(origSize);//, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		
		tmpImg.save("interpolate.jpg");

		//setPixmap(QPixmap::fromImage(img));//.scaled(200,200)));

		p.drawImage(0,0,tmpImg);
	}
	else
	if(m_gs->m_renderMode == MapGraphicsScene::RenderTriangles)
	{
		foreach(QString apMac, apMacToEssid.keys())
		{
			MapApInfo *info = m_gs->apInfo(apMac);

			QPointF center = info->point;

			foreach(SigMapValue *val, m_gs->m_sigValues)
				val->consumed = false;

			// Find first two closest points to center [0]
			// Make triangle
			// Find next cloest point
			// Make tri from last point, this point, center
			// Go on till all points done

			// Render first traingle
	// 		SigMapValue *lastPnt = findNearest(center, apMac);
	// 		if(lastPnt)
	// 		{
	// 			QImage tmpImg(origSize, QImage::Format_ARGB32_Premultiplied);
	// 			QPainter p2(&tmpImg);
	// 			p2.fillRect(tmpImg.rect(), Qt::transparent);
	// 			p2.end();
	//
	// 			SigMapValue *nextPnt = findNearest(lastPnt->point, apMac);
	// 			renderTriangle(&tmpImg, center, lastPnt, nextPnt, dx,dy, apMac);
	//
	// 			// Store this point to close the map
	// 			SigMapValue *endPoint = nextPnt;
	//
	// 			// Render all triangles inside
	// 			while((nextPnt = findNearest(lastPnt->point, apMac)) != NULL)
	// 			{
	// 				renderTriangle(&tmpImg, center, lastPnt, nextPnt, dx,dy, apMac);
	// 				lastPnt = nextPnt;
	// 				//lastPnt = 0;//DEBUG
	// 			}
	//
	// 			// Close the map
	// 			renderTriangle(&tmpImg, center, lastPnt, endPoint, dx,dy, apMac);
	//
	// 			p.setOpacity(1. / (double)(numAps) * (numAps - idx));
	// 			p.drawImage(0,0,tmpImg);
	//
	// 		}


			QImage tmpImg(origSize, QImage::Format_ARGB32_Premultiplied);
			QPainter p2(&tmpImg);
			p2.fillRect(tmpImg.rect(), Qt::transparent);
			p2.end();

			SigMapValue *lastPnt;
			while((lastPnt = m_gs->findNearest(center, apMac)) != NULL)
			{
				SigMapValue *nextPnt = m_gs->findNearest(lastPnt->point, apMac);
				m_gs->renderTriangle(&tmpImg, center, lastPnt, nextPnt, dx,dy, apMac);
			}

			p.setOpacity(.5); //1. / (double)(numAps) * (numAps - idx));
			p.drawImage(0,0,tmpImg);


			idx ++;
		}

	}
	else
	if(m_gs->m_renderMode == MapGraphicsScene::RenderRadial ||
	   m_gs->m_renderMode == MapGraphicsScene::RenderCircles)
	{
		/*
		#ifdef Q_OS_ANDROID
		int steps = 4 * 4 * 2;
		#else
		int steps = 4 * 4 * 4; //4 * 2;
		#endif
		*/
		int steps = m_gs->m_renderOpts.radialCircleSteps;

		double angleStepSize = 360. / ((double)steps);

		//int radius = 64*10/2;

		/*
		#ifdef Q_OS_ANDROID
		const double levelInc = .50; //100 / 1;// / 2;// steps;
		#else
		const double levelInc = .25; //100 / 1;// / 2;// steps;
		#endif
		*/
		double levelInc = 100. / ((double)m_gs->m_renderOpts.radialLevelSteps);

		double totalProgress = (100./levelInc) * (double)steps * apMacToEssid.keys().size();
		double progressCounter = 0.;

		(void)totalProgress; // avoid "unused" warnings...

		if(m_gs->m_renderMode == MapGraphicsScene::RenderCircles)
		{
			totalProgress = apMacToEssid.keys().size() * m_gs->m_sigValues.size();
		}

		foreach(QString apMac, apMacToEssid.keys())
		{
			MapApInfo *info = m_gs->apInfo(apMac);

			if(!info->marked)
			{
				//qDebug() << "SigMapRenderer::render(): [radidal] Unable to render signals for apMac:"<<apMac<<", AP not marked on MAP yet.";
				continue;
			}

			if(!info->renderOnMap)
			{
				qDebug() << "SigMapRenderer::render(): [radidal] User disabled render for "<<apMac;
				continue;
			}

			foreach(SigMapValue *val, m_gs->m_sigValues)
				val->renderDataDirty = true;

			QPointF center = info->point;

			double maxDistFromCenter = -1;
			double maxSigValue = 0.0;
			foreach(SigMapValue *val, m_gs->m_sigValues)
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


			//double circleRadius = maxDistFromCenter;
// 			double circleRadius = (1.0/maxSigValue) /** .75*/ * maxDistFromCenter;
			double circleRadius = maxDistFromCenter / maxSigValue;
			//maxDistFromCenter = circleRadius;
			//qDebug() << "SigMapRenderer::render(): circleRadius:"<<circleRadius<<", maxDistFromCenter:"<<maxDistFromCenter<<", maxSigValue:"<<maxSigValue;
			qDebug() << "render: "<<apMac<<": circleRadius: "<<circleRadius<<", center:"<<center;



			QRadialGradient rg;

			QColor centerColor = m_gs->colorForSignal(1.0, apMac);
			rg.setColorAt(0., centerColor);
			//qDebug() << "SigMapRenderer::render(): "<<apMac<<": sig:"<<1.0<<", color:"<<centerColor;

			if(m_gs->m_renderMode == MapGraphicsScene::RenderCircles)
			{
				if(!m_gs->m_renderOpts.multipleCircles)
				{
					foreach(SigMapValue *val, m_gs->m_sigValues)
					{
						if(val->hasAp(apMac))
						{
							double sig = val->signalForAp(apMac);
							QColor color = m_gs->colorForSignal(sig, apMac);
							rg.setColorAt(1-sig, color);

							//double dx = val->point.x() - center.x();
							//double dy = val->point.y() - center.y();
							//double distFromCenter = sqrt(dx*dx + dy*dy);

							//qDebug() << "SigMapRenderer::render(): "<<apMac<<": sig:"<<sig<<", color:"<<color<<", distFromCenter:"<<distFromCenter;


						}

						progressCounter ++;
						emit renderProgress(progressCounter / totalProgress);


					}

					rg.setCenter(center);
					rg.setFocalPoint(center);
					rg.setRadius(maxDistFromCenter);

					p.setOpacity(.75); //1. / (double)(numAps) * (numAps - idx));
					p.setOpacity(1. / (double)(numAps) * (numAps - idx));
					p.setBrush(QBrush(rg));
					//p.setBrush(Qt::green);
					p.setPen(QPen(Qt::black,1.5));
					//p.drawEllipse(0,0,iconSize,iconSize);
					p.setCompositionMode(QPainter::CompositionMode_Xor);
					p.drawEllipse(center, maxDistFromCenter, maxDistFromCenter);
					//qDebug() << "SigMapRenderer::render(): "<<apMac<<": center:"<<center<<", maxDistFromCenter:"<<maxDistFromCenter<<", circleRadius:"<<circleRadius;
				}
				else
				{

					SigMapRenderer_sort_apMac = apMac;
					SigMapRenderer_sort_center = center;

					QList<SigMapValue*> readings = m_gs->m_sigValues;

					// Sort the readings based on signal level so readings cloest to the antenna are drawn last
					qSort(readings.begin(), readings.end(), SigMapRenderer_sort_SigMapValue);

					foreach(SigMapValue *val, readings)
					{
						if(val->hasAp(apMac))
						{
							double sig = val->signalForAp(apMac);
							QColor color = m_gs->colorForSignal(sig, apMac);
	//						rg.setColorAt(1-sig, color);

							double dx = val->point.x() - center.x();
							double dy = val->point.y() - center.y();
	//  						double distFromCenter = sqrt(dx*dx + dy*dy);

							//qDebug() << "SigMapRenderer::render(): "<<apMac<<": sig:"<<sig<<", color:"<<color<<", distFromCenter:"<<distFromCenter;

							//p.drawEllipse(center, distFromCenter, distFromCenter);
	// 						rg.setCenter(center);
	// 						rg.setFocalPoint(center);
	// 						rg.setRadius(maxDistFromCenter);

							//p.setOpacity(.75); //1. / (double)(numAps) * (numAps - idx));
							p.setOpacity(.5); //1. / (double)(numAps) * (numAps - idx));
							if(m_gs->m_renderOpts.fillCircles)
								p.setBrush(color); //QBrush(rg));

							#ifdef Q_OS_ANDROID
							const double maxLineWidth = 7.5;
							#else
							const double maxLineWidth = 5.0;
							#endif

							if(m_gs->m_renderOpts.fillCircles)
								p.setPen(QPen(color.darker(400), (1.-sig)*maxLineWidth));
							else
								p.setPen(QPen(color, (1.-sig)*maxLineWidth));


							//p.setPen(QPen(Qt::black,1.5));

							p.drawEllipse(center, fabs(dx), fabs(dy));
						}

						progressCounter ++;
						emit renderProgress(progressCounter / totalProgress);
					}
				}

				//qDebug() << "SigMapRenderer::render(): "<<apMac<<": center:"<<center<<", maxDistFromCenter:"<<maxDistFromCenter;
			}
			else
			{

				#define levelStep2Point(levelNum,stepNum) QPointF( \
						(levelNum/100.*circleRadius) * cos(((double)stepNum) * angleStepSize * 0.0174532925) + center.x(), \
						(levelNum/100.*circleRadius) * sin(((double)stepNum) * angleStepSize * 0.0174532925) + center.y() )

				//qDebug() << "center: "<<center;


				QVector<QPointF> lastLevelPoints;
				for(double level=levelInc; level<=100.; level+=levelInc)
				{
					//qDebug() << "level:"<<level;
					QVector<QPointF> thisLevelPoints;
					QPointF lastPoint;
					for(int step=0; step<steps; step++)
					{
						QPointF realPoint = levelStep2Point(level,step);
						double renderLevel = m_gs->getRenderLevel(level,step * angleStepSize,realPoint,apMac,center,circleRadius);
						// + ((step/((double)steps)) * 5.); ////(10 * fabs(cos(step/((double)steps) * 359. * 0.0174532925)));// * cos(level/100. * 359 * 0.0174532925);

						QPointF here = levelStep2Point(renderLevel,step);

						if(step == 0) // && renderLevel > levelInc)
						{
							QPointF realPoint = levelStep2Point(level,(steps-1));

							double renderLevel = m_gs->getRenderLevel(level,(steps-1) * angleStepSize,realPoint,apMac,center,circleRadius);
							lastPoint = levelStep2Point(renderLevel,(steps-1));
							thisLevelPoints << lastPoint;
						}

						p.setOpacity(.8); //1. / (double)(numAps) * (numAps - idx));
						//p.setCompositionMode(QPainter::CompositionMode_Xor);

		//				p.setPen(QPen(centerColor.darker(), 4.0));
						#ifdef Q_OS_ANDROID
						const double maxLineWidth = 7.5;
						#else
						const double maxLineWidth = 15.0;
						#endif

						p.setPen(QPen(m_gs->colorForSignal((level)/100., apMac), (1.-(level/100.))*levelInc*maxLineWidth));

						p.drawLine(lastPoint,here);
						thisLevelPoints << here;

						lastPoint = here;

						progressCounter ++;
						emit renderProgress(progressCounter / totalProgress);
					}

					/*
					if(level > levelInc)
					{
						lastLevelPoints << thisLevelPoints;
						p.setBrush(m_gs->colorForSignal((level)/100., apMac));
						//p.drawPolygon(lastLevelPoints);
					}
					*/

					lastLevelPoints = thisLevelPoints;
				}



		// 		p.setCompositionMode(QPainter::CompositionMode_SourceOver);
		//
		// 		// Render lines to signal locations
		// 		p.setOpacity(.75);
		// 		p.setPen(QPen(centerColor, 1.5));
		// 		foreach(SigMapValue *val, m_gs->m_sigValues)
		// 		{
		// 			if(val->hasAp(apMac))
		// 			{
		// // 				double sig = val->signalForAp(apMac);
		// // 				QColor color = m_gs->colorForSignal(sig, apMac);
		// // 				rg.setColorAt(1-sig, color);
		// //
		// // 				qDebug() << "SigMapRenderer::render(): "<<apMac<<": sig:"<<sig<<", color:"<<color;
		//
		// 				p.setPen(QPen(centerColor, val->signalForAp(apMac) * 10.));
		// 				//p.drawLine(center, val->point);
		//
		// 				/// TODO Use radial code below to draw the line to the center of the circle for *this* AP
		// 				/*
		// 				#ifdef Q_OS_ANDROID
		// 					const int iconSize = 64;
		// 				#else
		// 					const int iconSize = 32;
		// 				#endif
		//
		// 				const double iconSizeHalf = ((double)iconSize)/2.;
		//
		// 				int numResults = results.size();
		// 				double angleStepSize = 360. / ((double)numResults);
		//
		// 				QRectF boundingRect;
		// 				QList<QRectF> iconRects;
		// 				for(int resultIdx=0; resultIdx<numResults; resultIdx++)
		// 				{
		// 					double rads = ((double)resultIdx) * angleStepSize * 0.0174532925;
		// 					double iconX = iconSizeHalf/2 * numResults * cos(rads);
		// 					double iconY = iconSizeHalf/2 * numResults * sin(rads);
		// 					QRectF iconRect = QRectF(iconX - iconSizeHalf, iconY - iconSizeHalf, (double)iconSize, (double)iconSize);
		// 					iconRects << iconRect;
		// 					boundingRect |= iconRect;
		// 				}
		//
		// 				boundingRect.adjust(-1,-1,+2,+2);
		// 				*/
		// 			}
		// 		}
			}

			idx ++;
		}
	}

	//mapImg.save("mapImg1.jpg");
	p.end();


	//m_gs->m_sigMapItem->setPixmap(QPixmap::fromImage(mapImg.scaled(origSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
	//emit renderComplete(mapImg);
// 	QPicture pic;
// 	QPainter pp(&pic);
// 	pp.drawImage(0,0,mapImg);
// 	pp.end();

	emit renderComplete(pic);
}





void MapGraphicsScene::renderTriangle(QImage *img, SigMapValue *a, SigMapValue *b, SigMapValue *c, double dx, double dy, QString apMac)
{
	if(!a || !b || !c)
		return;

/*	QVector<QPointF> points = QVector<QPointF>()
		<< QPointF(10, 10)
		<< QPointF(imgWidth - 10, imgHeight / 2)
		<< QPointF(imgWidth / 2,  imgHeight - 10);

	QList<QColor> colors = QList<QColor>()
		<< Qt::red
		<< Qt::green
		<< Qt::blue;*/

	QVector<QPointF> points = QVector<QPointF>()
		<< QPointF(a->point.x() * dx, a->point.y() * dy)
		<< QPointF(b->point.x() * dx, b->point.y() * dy)
		<< QPointF(c->point.x() * dx, c->point.y() * dy);

	QList<QColor> colors = QList<QColor>()
		<< colorForSignal(a->signalForAp(apMac), apMac)
		<< colorForSignal(b->signalForAp(apMac), apMac)
		<< colorForSignal(c->signalForAp(apMac), apMac);

	qDebug() << "MapGraphicsScene::renderTriangle[1]: "<<apMac<<": "<<points;
	if(!fillTriColor(img, points, colors))
		qDebug() << "MapGraphicsScene::renderTriangle[1]: "<<apMac<<": Not rendered";

// 	QPainter p(img);
// 	//p.setBrush(colors[0]);
// 	p.setPen(QPen(colors[2],3.0));
// 	p.drawConvexPolygon(points);
// 	p.end();

}

void MapGraphicsScene::renderTriangle(QImage *img, QPointF center, SigMapValue *b, SigMapValue *c, double dx, double dy, QString apMac)
{
	if(!b || !c)
		return;

/*	QVector<QPointF> points = QVector<QPointF>()
		<< QPointF(10, 10)
		<< QPointF(imgWidth - 10, imgHeight / 2)
		<< QPointF(imgWidth / 2,  imgHeight - 10);

	QList<QColor> colors = QList<QColor>()
		<< Qt::red
		<< Qt::green
		<< Qt::blue;*/

	QVector<QPointF> points = QVector<QPointF>()
		<< QPointF(center.x() * dx, center.y() * dy)
		<< QPointF(b->point.x() * dx, b->point.y() * dy)
		<< QPointF(c->point.x() * dx, c->point.y() * dy);

	QList<QColor> colors = QList<QColor>()
		<< colorForSignal(1.0, apMac)
		<< colorForSignal(b->signalForAp(apMac), apMac)
		<< colorForSignal(c->signalForAp(apMac), apMac);

	qDebug() << "MapGraphicsScene::renderTriangle[2]: "<<apMac<<": "<<points;
	if(!fillTriColor(img, points, colors))
		qDebug() << "MapGraphicsScene::renderTriangle[2]: "<<apMac<<": Not rendered";

// 	QPainter p(img);
// 	//p.setBrush(colors[0]);
// 	p.setPen(QPen(colors[2],3.0));
// 	p.drawConvexPolygon(points);
// 	p.end();

}

double MapGraphicsScene::getRenderLevel(double level,double angle,QPointF realPoint,QString apMac,QPointF center,double circleRadius)
{
	foreach(SigMapValue *val, m_sigValues)
	{
		if(val->hasAp(apMac))
		{

			// First, conver the location of this SigMapValue to an angle and "level" (0-100%) in relation to the center
			double testLevel;
			double testAngle;
			if(val->renderDataDirty ||
			   val->renderCircleRadius != circleRadius)
			{
				QPointF calcPoint = val->point - center;

				testLevel = sqrt(calcPoint.x() * calcPoint.x() + calcPoint.y() * calcPoint.y());
				testAngle = atan2(calcPoint.y(), calcPoint.x());// * 180 / Pi;

				// Convert the angle from radians in the range of [-pi,pi] to degrees
				testAngle = testAngle/Pi2 * 360;
				if(testAngle < 0) // compensate for invalid Quad IV angles in relation to points returned by sin(rads(angle)),cos(rads(angle))
					testAngle += 360;

				// Conver the level to a 0-100% value
				testLevel = (testLevel / circleRadius) * 100.;

				val->renderCircleRadius = circleRadius;
				val->renderLevel = testLevel;
				val->renderAngle = testAngle;
			}
			else
			{
				testLevel = val->renderLevel;
				testAngle = val->renderAngle;
			}

			// Get the level of signal read for the current AP at this locaiton
			double signalLevel = val->signalForAp(apMac) * 100.;

			// Get the difference in angle for this reading from the current angle,
			// and the difference in "level" of this reading from the current level.
			// Because:
			// Only values within X degrees and Y% of current point
			// will affect this point
			double angleDiff = fabs(testAngle - angle);
			double levelDiff = fabs(testLevel - level);

			// Now, calculate the straight-line distance of this reading to the current point we're rendering
			QPointF calcPoint = val->point - realPoint;
			double lineDist = sqrt(calcPoint.x() * calcPoint.x() + calcPoint.y() * calcPoint.y());
			//const double lineWeight = 200.; // NOTE arbitrary magic number
			double lineWeight = (double)m_renderOpts.radialLineWeight;

			// Calculate the value to adjust the render level by.
			double signalLevelDiff = (testLevel - signalLevel)    // Adjustment is based on the differences in the current render level vs the signal level read at this point
						* (levelDiff / 100.)	      // Apply a weight based on the difference in level (the location its read at vs the level we're rendering - points further away dont affect as much)
						* (1./(lineDist/lineWeight)); // Apply a weight based on the straight-line difference from this location to the signal level

			// The whole theory we're working with here is that the rendering algorithm will go in a circle around the center,
			// starting at level "0", with the idea that the "level" is the "ideal signal level" - incrementing outward with
			// the level being expressed as a % of the circle radius. The rendering algo makes one complete circle at each
			// level, calculating the ideal point on the circle for each point at that level.

			// To express the signal readings, at each point on the circle on the current level, we calculate a "render level", wherein we adjust the current "level"
			// (e.g. the radius, or distance for that point from the circle's center) based on the formula above.

			// The forumla above is designed to take into account the signal reading closest to the current point, moving the point closer to the circle's center or further away
			// if the signal reading is stronger than "it should be" at that level or weaker"

			// By "stronger (or weaker) than it should be", we mean signal reading at X% from the circles center reads Y%, where ||X-Y|| > 0

			// To show the effect of the readings, we allow the signal markers to "warp" the circle - readings stronger than they should be closer to the circle
			// will "pull in" the appros level (a 50% reading at 10% from the circle's center will "pull in" the "50% line")

			// Now, inorder to extrapolate the effect of the reading and interpolate it across multiple points in the circle, and to factor in the warping
			// effect of nearby readings (e.g. to bend the lineds toward those values, should they deviate from their ideal readings on the circle),
			// we apply several weights, based on the difference in distance from the circle's center the reading is vs the current level,
			// as well as the straight-line distance from the reading in question to the current "ideal" rendering point.

			// Then we sum "adjustment value" (signalLevelDiff) changes onto the level value, so that oposing readings can cancel out each other appropriatly.

			// The result is a radial flow diagram, warped to approximately fit the signal readings, and appears to properly model apparent real-world signal propogations.

			// This rendering model can be tuned/tweaked in a number of locations:
			//	- The "angleDiff" and "levelDiff" comparrison values, below - These specifiy the maximum allowable difference in
			//	  angle that the current SigMapValue and max allowable diff level that the SigMapValue can be inorder for it to have any influence whatosever on the current level
			//	- The lineDist divisor above in the third parenthetical part of the signalLevelDiff equation - the value should be *roughly* what the max
			//	  straight-line dist is that can be expected by allowing the given angleDiff/levelDiff (below) - though the value is inverted (1/x) in order to smooth
			//	  the effects of this particular value on the overall adjustment
			//	Additionally, rendering quality and speed can be affected in the SigMapRenderer::render() function by adjusting:
			//	- "int steps" variable (number of circular sections to render)
			//	- "levelInc" variable - the size of steps from 0-100% to take (currently, it's set to .25, making 400 total steps from 0-100 - more steps, more detailed rendering
			//	- You can also tweak the line width algorithm right before the call to p.drawLine, as well as the painter opacity to suit tastes

			//qDebug() << "getRenderLevel: val "<<val->point<<", signal for "<<apMac<<": "<<(int)signalLevel<<"%, angleDiff:"<<angleDiff<<", levelDiff:"<<levelDiff<<", lineDist:"<<lineDist<<", signalLevelDiff:"<<signalLevelDiff;

			/*
			if(angleDiff < 135. && // 1/3rd of 360*
			   levelDiff <  33.)   // 1/3rd of 100 levels
			*/
			if(angleDiff < m_renderOpts.radialAngleDiff &&
			   levelDiff < m_renderOpts.radialLevelDiff)
			{
				//double oldLev = level;
				level += signalLevelDiff;

				//qDebug() << "\t - level:"<<level<<" (was:"<<oldLev<<"), signalLevelDiff:"<<signalLevelDiff;

				//qDebug() << "getRenderLevel: "<<center<<" -> "<<realPoint.toPoint()<<": \t " << (round(angle) - round(testAngle)) << " \t : "<<level<<" @ "<<angle<<", calc: "<<testLevel<<" @ "<<testAngle <<", "<<calcPoint;
			}
		}
	}

	if(level < 0)
		level = 0;

	return level;
}
