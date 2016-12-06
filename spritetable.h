#ifndef SPRITETABLE_H
#define SPRITETABLE_H
#include <QTableWidget>
#include "commandchain.h"

class ImageView;

class SpriteTable : public QTableWidget
{
	Q_OBJECT

	typedef QTableWidget super;

public:
	CommandList command_list;

	static bool clipboardHasImage();

	explicit SpriteTable(QWidget *parent = Q_NULLPTR);
    SpriteTable(int rows, int columns, QWidget *parent = Q_NULLPTR);
    ~SpriteTable();

	bool dropMimeData(int row, int column, const QMimeData * data, Qt::DropAction action);
	Qt::DropActions supportedDropActions() const;

	bool editReplaceImage(QImage &image, int row, int column);
	bool editReplaceImage(QImage &image);
	void editInsertRow();
	bool editClear(int row, int column);
	bool editDelete(int row, int column);

	static
	QImage imageFromeMime(QMimeData * data);
	void mousePressEvent(QMouseEvent * event);

	void dropEvent(QDropEvent * event);
	void dragMoveEvent(QDragMoveEvent * event);
	void dragEnterEvent(QDragEnterEvent * event);


	void toolsClearRow(int row);
	void toolsPruneC2Sprites(char part);
	void toolsPruneC3Sprites(char part);
	void toolsSelectEveryNthSprite(int offset, int N);

public slots:
	void editUndo();
	void editRedo();

	bool editCopy();
	bool editPaste();
	bool editCut()
	{
		return editCopy() && editDelete();
	}

	bool editDelete();

	bool editImport();
	bool editExport();

	void menuInsert();
	void menuSelect();
	void menuClear();
	void menuDelete();

	void toolsInterpolateColor();
	void toolsBlurAlpha();
	void toolsScaleImages();

	void toolsRearrangeRotationOrder();

	void toolsSelectNthSprite();
	void toolsSelectEveryNthSprite();

	void toolsClearNthRow();
	void toolsClearEveryNthRow();

	void toolsRemoveNthRow();
	void toolsRemoveEveryNthRow();

	void customMenuRequested(QPoint pos);

private:
	bool loadImage(const QString &, QImage&);
	bool saveImage(const QString &, const QImage&);
	QImage openImage();
	QImage imageFromClipboard();
};

#endif // SPRITETABLE_H
