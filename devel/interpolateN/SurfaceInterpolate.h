#ifndef SurfaceInterpolate_H
#define SurfaceInterpolate_H
#include <QtGui>

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

namespace SurfaceInterpolate 
{
	class Interpolator
	{
	public:
		Interpolator() { resetSettings(); }
		
		void resetSettings()
		{
			m_pointSizeCutoff	= 50; // less inputs than this - use rectangle-tesselation to generate point cloud, greater than this, use grid and IDW to generate point cloud
			m_gridNumStepsX		= 30; // size of grid if using grid and IDW
			m_gridNumStepsY		= 30;
			m_gridStepSizeX		= 0.;
			m_gridStepSizeY		= 0.;
			m_nearbyDistance	= 0.; // when using IDW, points closer than m_nearbyDistance in liner distance are weighted to produce the value. m_nearbyDistance is assumed squared 
			m_autoNearby		= true; // auto, defined as 1/4*width() * 1/4*height()
			m_scaleValues		= true; // scale values from 0-max to 0-1
		}
		
		int pointSizeCutoff()	{ return m_pointSizeCutoff; }
		int gridNumStepsX()	{ return m_gridNumStepsX; }
		int gridNumStepsY()	{ return m_gridNumStepsY; }
		float gridStepSizeX()	{ return m_gridStepSizeX; }
		float gridStepSizeY()	{ return m_gridStepSizeY; }
		float nearbyDistance()	{ return m_nearbyDistance; }
		bool autoNearby()	{ return m_autoNearby; }
		bool scaleValues()	{ return m_scaleValues; }
		
		void setPointSizeCutoff(int value)	{ m_pointSizeCutoff = value; }
		void setGridNumStepsX(int value)	{ m_gridNumStepsX = value; }
		void setGridNumStepsY(int value)	{ m_gridNumStepsY = value; }
		void setGridStepSizeX(float value)	{ m_gridStepSizeX = value; }
		void setGridStepSizeY(float value)	{ m_gridStepSizeY = value; }
		void setNearbyDistance(float value)	{ m_nearbyDistance = value; m_autoNearby = value < 0; }
		void setAutoNearby(bool flag)		{ m_autoNearby = flag; }
		void setScaleValues(bool flag)		{ m_scaleValues = flag; }
		
		double interpolateValue(QPointF point, QList<qPointValue> inputs);
		QList<qQuadValue> generateQuads(QList<qPointValue> points, bool forceGrid=false);
		QImage renderPoints(QList<qPointValue> points, QSize renderSize = QSize(), bool renderLines=false, bool renderPointValues=false);
		bool write3dSurfaceFile(QString objFile, QList<qPointValue> points, QSizeF renderSize, double valueScale = -1, QString mtlFile="", double xres=10, double yres=10);
		
	protected:
		QList<qPointValue> testLine(QList<qPointValue> inputs, QPointF p1, QPointF p2, int dir);
		
		int m_pointSizeCutoff;
		int m_gridNumStepsX;
		int m_gridNumStepsY;
		float m_gridStepSizeX;
		float m_gridStepSizeY;
		float m_nearbyDistance;
		float m_autoNearby;
		bool m_scaleValues;
		
		QRectF m_currentBounds;
		
		QList<int> m_pointsUsed;
		
		int pointKey(QPointF);
		bool isPointUsed(QPointF);
		void markPointUsed(QPointF);
	};

};

#endif
