#ifndef ImageUtils_H
#define ImageUtils_H

#include <QtGui>

class ImageUtils
{
public:
	static QImage blurred(const QImage& image, const QRect& rect, int radius);

	// Modifies 'image'
	static void blurImage(QImage& image, int radius, bool highQuality = false);
	static QRectF blurredBoundingRectFor(const QRectF &rect, int radius);
	static QSizeF blurredSizeFor(const QSizeF &size, int radius);

	static QImage addDropShadow(QImage img, double shadowSize = 16.);
};

#endif
