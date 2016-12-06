#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H
#include <QSharedPointer>
#include <QPoint>
#include <QTableWidgetItem>

class QTableWidget;

class ImageView : public QTableWidgetItem
{
typedef QTableWidgetItem super;
	const int row, column;

	std::vector<ImageView *> map;

public:
	explicit ImageView(int row, int col);
	explicit ImageView(QTableWidget *table, int row, int col);
	~ImageView();

	QImage image;
	QRect bounds;
	QImage original;

	void setThumbnail();

	bool setImage(QImage img);

	void readImage(FILE * file, short w, short h);
	void writeImage(FILE *file, uint32_t * offset);

	void initialize(QTableWidget *table);
	void deinitialize();
};

#endif // IMAGEVIEW_H
