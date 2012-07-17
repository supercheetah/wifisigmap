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

double quadInterpolate(QList<qPointValue> corners, double x, double y)
{
	// Very simple implementation of http://en.wikipedia.org/wiki/Bilinear_interpolation
	double w = corners[2].point.x() - corners[0].point.x();
	double h = corners[2].point.y() - corners[0].point.y();

	double fr1 = (corners[2].point.x() - x) / w * corners[3].value
		   + (x - corners[0].point.x()) / w * corners[2].value;

	double fr2 = (corners[2].point.x() - x) / w * corners[0].value
		   + (x - corners[0].point.x()) / w * corners[1].value;

	double p   = (corners[2].point.y() - y) / h * fr1
		   + (y - corners[0].point.y()) / h * fr2;

	//qDebug() << "quadInterpolate: "<<x<<" x "<<y<<": "<<p<<" (fr1:"<<fr1<<",fr2:"<<fr2<<",w:"<<w<<",h:"<<h<<")";
	return p;
}

MainWindow::MainWindow()
{
	QImage img(800,600, QImage::Format_ARGB32_Premultiplied);

	QList<qPointValue> corners = QList<qPointValue>()
		<< qPointValue(QPointF(0,0), 1.0)
		<< qPointValue(QPointF(img.width(),0), 0.5)
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

	setPixmap(QPixmap::fromImage(img));
}


