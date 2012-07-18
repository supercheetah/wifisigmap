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
	return QColor::fromHsv((1-v) * 359, 255, 255);
}

class qPointValue {
public:
	qPointValue(QPointF p= QPointF(), double v=0.0) : point(p), value(v) {}
	
	QPointF point;
	double value;
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

double quadInterpolate(QList<qPointValue> corners, double x, double y)
{
	double w = corners[2].point.x() - corners[0].point.x();
	double h = corners[2].point.y() - corners[0].point.y();
	
	bool useBicubic = true;
	if(useBicubic)
	{
		// Use bicubic impl from http://www.paulinternet.nl/?page=bicubic
		double unitX = (corners[2].point.x() - x) / w;
		double unitY = (corners[2].point.y() - y) / h;
		
		double c1 = corners[3].value,
		c2 = corners[0].value,
		c3 = corners[1].value,
		c4 = corners[2].value;
	
	//  	double cTopW = (c2-c1);
	//  	double cBotW = (c3-c4);
	//  	double cRH   = (c3-c2);
	//  	double cLH   = (c4-c1);
		
		double p[4][4] = 
		{
			{c1,c1,c2,c2},
			{c1,c1,c2,c2},
			{c4,c4,c3,c3},
			{c4,c4,c3,c3}
		};
		
		double p2 = bicubicInterpolate(p, unitX, unitY);
		return p2;
	}
	else
	{
		// Very simple implementation of http://en.wikipedia.org/wiki/Bilinear_interpolation
		double fr1 = (corners[2].point.x() - x) / w * corners[3].value
			   + (x - corners[0].point.x()) / w * corners[2].value;
	
		double fr2 = (corners[2].point.x() - x) / w * corners[0].value
			   + (x - corners[0].point.x()) / w * corners[1].value;
	
		double p1  = (corners[2].point.y() - y) / h * fr1
			   + (y - corners[0].point.y()) / h * fr2;
	
		//qDebug() << "quadInterpolate: "<<x<<" x "<<y<<": "<<p<<" (fr1:"<<fr1<<",fr2:"<<fr2<<",w:"<<w<<",h:"<<h<<")";
		return p1;
	}
}

MainWindow::MainWindow()
{
	QImage img(800,800, QImage::Format_ARGB32_Premultiplied);

	QList<qPointValue> corners = QList<qPointValue>()
		<< qPointValue(QPointF(0,0), 1.0)
		<< qPointValue(QPointF(img.width(),0), 0.0)
		<< qPointValue(QPointF(img.width(),img.height()), 1.0)
		<< qPointValue(QPointF(0,img.height()), 0.0);
/*
	foreach(qPointValue v, corners)
	{
		v.point.rx() *= img.width();
		v.point.ry() *= img.height();

		qDebug() << "corner: "<<v.point<<", value:"<<v.value;
	}*/

	for(int y=0; y<img.height(); y++)
	{
		QRgb* scanline = (QRgb*)img.scanLine(y);
		for(int x=0; x<img.width(); x++)
		{
			double value = quadInterpolate(corners, (double)x, (double)y);
			QColor color = colorForValue(value);
			scanline[x] = color.rgba();
		}
	}
	
	img.save("interpolate.jpg");

	setPixmap(QPixmap::fromImage(img));
}


