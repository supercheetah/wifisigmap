#ifndef SigMapRenderer_H
#define SigMapRenderer_h

#include <QtGui>

class MapGraphicsScene;

class SigMapRenderer : public QObject
{
	Q_OBJECT
public:
	SigMapRenderer(MapGraphicsScene* gs);

public slots:
	void render();

signals:
	void renderProgress(double); // 0.0-1.0
	void renderComplete(QPicture); //QImage);
	//void renderComplete(QImage);

protected:
	MapGraphicsScene *m_gs;
};

#endif
