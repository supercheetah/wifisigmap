#ifndef LongPressSpinner_H
#define LongPressSpinner_H

#include <QtGui>

class LongPressSpinner : public QObject, public QGraphicsItem
{
	Q_OBJECT
	Q_INTERFACES(QGraphicsItem)
public:
	LongPressSpinner();

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public slots:
	void setVisible(bool flag);
	void setGoodPress(bool flag = true);
	void setProgress(double); // [0,1]

protected slots:
	void goodPressExpire();
	void fadeTick();

protected:
	bool m_goodPressFlag;
	double m_progress;
	QRectF m_boundingRect;
	QTimer m_goodPressTimer;

};

#endif
