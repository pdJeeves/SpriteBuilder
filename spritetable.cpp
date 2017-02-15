#include "spritetable.h"
#include <QMimeData>
#include <QImageReader>
#include <QMouseEvent>
#include <QDrag>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include "spritebuilder.h"
#include "imageview.h"

SpriteTable::SpriteTable(QWidget *parent)
	: QTableWidget(parent)
{
	setAcceptDrops(true);
	verticalHeader()->setSectionsMovable(true);
	verticalHeader()->setDragEnabled(true);
	verticalHeader()->setDragDropMode(QHeaderView::InternalMove);

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customMenuRequested(QPoint)));
}

SpriteTable::SpriteTable(int rows, int columns, QWidget *parent)
	: QTableWidget(rows, columns, parent)
{
	setAcceptDrops(true);
}

SpriteTable::~SpriteTable()
{
}

QImage SpriteTable::imageFromeMime(QMimeData * data)
{
	if (data->hasImage()) {
		QPixmap pixmap = qvariant_cast<QPixmap>(data->imageData());
		return pixmap.toImage();
	}
	else if(data->hasText())
	{
		QString filename = data->text();
		QImageReader reader(filename);
		reader.setAutoTransform(true);
		auto newImage = reader.read();

		if (!newImage.isNull())
		{
			return newImage;
		}
	}

	return QImage();
}


void SpriteTable::dragMoveEvent(QDragMoveEvent * event)
{
	event->accept();
}

void SpriteTable::dragEnterEvent(QDragEnterEvent * event)
{
	event->acceptProposedAction();
}

void SpriteTable::dropEvent(QDropEvent * event)
{
	QModelIndex _pos = indexAt(event->pos());

	int row		= _pos.row();
	int column  = _pos.column();

	if(column == -1)
	{
		return;
	}
	else if(row == -1)
	{
		insertRow(rowCount());
		row = rowCount()-1;
	}

	setCurrentCell(row, column);
	event->acceptProposedAction();
}

bool SpriteTable::dropMimeData(int row, int column, const QMimeData * data, Qt::DropAction)
{
	auto image = imageFromeMime(const_cast<QMimeData *>(data));

	if(image.isNull())
	{
		return false;
	}

	return editReplaceImage(image, row, column);
}

Qt::DropActions SpriteTable::supportedDropActions() const
{
	return Qt::MoveAction | Qt::CopyAction;
}


QRect calculateBoundingBox(const QImage & img);

QImage & correctImage(QImage & image)
{
	if(image.isNull())
	{
		return image;
	}

	if(((image.width()+3) & 0xFFFC) == image.width()
	&&((image.height()+3) & 0xFFFC) == image.height())
	{
		return image;
	}

	QImage img((image.width()+3) & 0xFFFC, (image.height()+3) & 0xFFFC, QImage::Format_ARGB32_Premultiplied);
	img.fill(0);

	for(int y = 0; y < image.height(); ++y)
	{
		for(int x = 0; x < image.width(); ++x)
		{
			img.setPixel(x, y, image.pixel(x, y));
		}
	}

	image = std::move(img);
	return image;
}

bool SpriteTable::editReplaceImage(QImage & image, int row, int column)
{
	auto n_box = calculateBoundingBox(image);
	for(uint8_t j = 0; j < columnCount(); ++j)
	{
		if(j == column)
		{
			continue;
		}

		auto it = dynamic_cast<ImageView*>(item(row, j));
		if(it && !it->image.isNull())
		{
			if(it->bounds != n_box)
			{
				return false;
			}
		}
	}

	command_list.push(this, new SetCommand(this, row, column, image));
	return true;
}

bool SpriteTable::editReplaceImage(QImage & image)
{
	return editReplaceImage(image, currentRow(), currentColumn());
}

bool SpriteTable::editDelete(int row, int column)
{
	if(row < 0 || column < 0)
	{
		return false;
	}

	if(item(row, column))
	{
		command_list.push(this, new SetCommand(this, row, column));
		return true;
	}

	for(uint8_t i = 0; i < columnCount(); ++i)
	{
		auto it = dynamic_cast<ImageView*>(item(row, i));

		if(it && !it->image.isNull())
		{
			return false;
		}
	}

	command_list.push(this, new DeletionCommand(this, row));

	return true;
}

bool SpriteTable::editClear(int row, int column)
{
	if(row < 0 || column < 0)
	{
		return false;
	}

	if(item(row, column))
	{
		command_list.push(this, new SetCommand(this, row, column));
		return true;
	}

	return false;
}


void SpriteTable::mousePressEvent(QMouseEvent * event)
{
	super::mousePressEvent(event);
	if (event->button() != Qt::LeftButton)
	{
		return;
	}

	QModelIndex _pos = indexAt(event->pos());

	int row		= _pos.row();
	int column  = _pos.column();

	if(row == -1 || column == -1)
	{
		return;
	}

	auto image = dynamic_cast<ImageView*>(item(row, column));
	if(image == 0L || image->image.isNull())
	{
		return;
	}

	QDrag *drag = new QDrag(this);
	QMimeData *mimeData = new QMimeData;
	mimeData->setText("dummy");
	drag->setMimeData(mimeData);
	Qt::DropAction dropAction = drag->exec(Qt::MoveAction | Qt::CopyAction, Qt::MoveAction);

	auto n_box = calculateBoundingBox(image->image);
	for(uint8_t j = 0; j < columnCount(); ++j)
	{
		if(j == column)
		{
			continue;
		}

		auto it = dynamic_cast<ImageView*>(item(currentRow(), j));
		if(it && !it->image.isNull())
		{
			if(it->bounds != n_box)
			{
				return;
			}
		}
	}

	if(dropAction == Qt::MoveAction)
	{
		auto action = new GroupCommand();
		action->push_back(new SetCommand(this, row, column));
		action->push_back(new SetCommand(this, currentRow(), currentColumn(), image->image));
		command_list.push(this, action);
	}
	else if(dropAction == Qt::CopyAction)
	{
		editReplaceImage(image->image);
	}
}

QImage blur_colors(const QImage & original, int blur_iterations);

void SpriteTable::toolsInterpolateColor()
{
	if(rowCount() == 0)
	{
		return;
	}

	bool okay = 0;
	int N = QInputDialog::getInt(this, tr("Interpolate Color"), tr("How many iterations?"), 5, 1, 10, 1, &okay);

	if(!okay)
	{
		return;
	}

	auto action = new GroupCommand();
	for(int i = 0; i < rowCount(); ++i)
	{
		ImageView * img = dynamic_cast<ImageView*>(item(i, 3));

		if(!img || img->image.isNull())
		{
			continue;
		}

		action->push_back(new SetCommand(this, i, 3, blur_colors(img->image, N)));
	}

	if(action->size())
	{
		command_list.push(this, action);
	}
	else
	{
		delete action;
	}
}

QImage blur_alpha(const QImage & image, int alpha_iterations);

void SpriteTable::toolsBlurAlpha()
{
	if(rowCount() == 0)
	{
		return;
	}

	bool okay = 0;
	int N = QInputDialog::getInt(this, tr("Blur Alpha"), tr("How many iterations?"), 2, 1, 9, 1, &okay);

	if(!okay)
	{
		return;
	}

	auto action = new GroupCommand();
	for(int i = 0; i < rowCount(); ++i)
	{
		ImageView * img = dynamic_cast<ImageView*>(item(i, 3));

		if(!img || img->image.isNull())
		{
			continue;
		}

		action->push_back(new SetCommand(this, i, 3, blur_alpha(img->image, N)));
	}

	if(action->size())
	{
		command_list.push(this, action);
	}
	else
	{
		delete action;
	}
}

QImage double_image(QImage image);

void SpriteTable::toolsScaleImages()
{
	if(rowCount() == 0)
	{
		return;
	}

	auto action = new GroupCommand();
	for(int i = 0; i < rowCount(); ++i)
	{
		for(int j = 0; j < columnCount(); ++j)
		{
			ImageView * img = dynamic_cast<ImageView*>(item(i, j));

			if(!img || img->image.isNull())
			{
				continue;
			}

			action->push_back(new SetCommand(this, i, j, double_image(img->image)));
		}
	}

	if(action->size())
	{
		command_list.push(this, action);
	}
	else
	{
		delete action;
	}
}


void SpriteTable::toolsRearrangeRotationOrder()
{
	if(rowCount() % 4 != 0)
	{
		QMessageBox::information(this, tr("Rearrange Rotation Order"), tr("Unable to rearrange items, (row count is not a multiple of 4)."));
	}

	command_list.push(this, new RearrangeCommand());
}

void SpriteTable::toolsSelectEveryNthSprite(int offset, int N)
{
	auto command = new GroupCommand();


	for(int i = 0, k = 0; i < rowCount(); ++i)
	{
		if((i - offset) % N != 0)
		{
			++k;
			continue;
		}

		for(int j = 0; j < columnCount(); ++j)
		{
			if(item(i, j))
			{
				command->push_back(new SetCommand(this, k, j));
			}
		}

		command->push_back(new DeletionCommand(this, k));
	}

	if(command->size())
	{
		command_list.push(this, command);
	}
	else
	{
		delete command;
	}
}

void SpriteTable::toolsClearRow(int row)
{
	auto command = new GroupCommand();

	for(int i = 0; i < columnCount(); ++i)
	{
		if(item(row, i))
		{
			command->push_back(new SetCommand(this, row, i));
		}
	}

	if(command->size())
	{
		command_list.push(this, command);
	}
	else
	{
		delete command;
	}
}

void SpriteTable::toolsClearNthRow()
{
	if(rowCount() == 0)
	{
		return;
	}

	bool okay = 0;
	int N = QInputDialog::getInt(this, tr("Remove Nth Row"), tr("Offset = ?"), 1, 1, rowCount(), 1, &okay)-1;

	if(!okay)
	{
		return;
	}

	toolsClearRow(N);
}

void SpriteTable::toolsClearEveryNthRow()
{
	if(rowCount() == 0)
	{
		return;
	}

	bool okay = 0;
	int offset = QInputDialog::getInt(this, tr("Remove Nth Row"), tr("Offset = ?"), 1, 1, rowCount(), 1, &okay)-1;
	if(!okay) { return; }
	int N = QInputDialog::getInt(this, tr("Remove Nth Row"), tr("N = ?"), 0, 0, rowCount()-1, 1, &okay);
	if(!okay) { return; }

	auto command = new GroupCommand();

	for(int i = offset; i < rowCount(); i += N)
	{
		for(int j = 0; j < columnCount(); ++j)
		{
			if(item(i, j))
			{
				command->push_back(new SetCommand(this, i, j));
			}
		}
	}

	if(command->size())
	{
		command_list.push(this, command);
	}
	else
	{
		delete command;
	}
}

void SpriteTable::toolsRemoveNthRow()
{
	if(rowCount() == 0)
	{
		return;
	}

	bool okay = 0;
	int N = QInputDialog::getInt(this, tr("Remove Nth Row"), tr("Offset = ?"), 1, 1, rowCount(), 1, &okay)-1;

	if(!okay)
	{
		return;
	}

	setCurrentCell(N, currentColumn());
	menuDelete();
}

void SpriteTable::toolsRemoveEveryNthRow()
{
	if(rowCount() == 0)
	{
		return;
	}

	bool okay = 0;
	int offset = QInputDialog::getInt(this, tr("Remove Nth Row"), tr("Offset = ?"), 1, 1, rowCount(), 1, &okay)-1;
	if(!okay) { return; }
	int N = QInputDialog::getInt(this, tr("Remove Nth Row"), tr("N = ?"), 0, 0, rowCount()-1, 1, &okay);
	if(!okay) { return; }

	auto command = new GroupCommand();

	for(int i = offset, k = offset; i < rowCount(); i += N, k += N-1)
	{
		for(int j = 0; j < columnCount(); ++j)
		{
			if(item(i, j))
			{
				command->push_back(new SetCommand(this, k, j));
			}
		}

		command->push_back(new DeletionCommand(this, k));
	}

	if(command->size())
	{
		command_list.push(this, command);
	}
	else
	{
		delete command;
	}
}

void SpriteTable::toolsSelectNthSprite()
{
	if(rowCount() == 0)
	{
		return;
	}

	bool okay = 0;
	int N = QInputDialog::getInt(this, tr("Remove Nth Row"), tr("Offset = ?"), 1, 1, rowCount(), 1, &okay)-1;

	if(!okay)
	{
		return;
	}

	setCurrentCell(N, currentColumn());
	menuSelect();
}


void SpriteTable::toolsSelectEveryNthSprite()
{
	if(rowCount() == 0)
	{
		return;
	}

	bool okay = 0;
	int offset = QInputDialog::getInt(this, tr("Remove Nth Row"), tr("Offset = ?"), 1, 1, rowCount(), 1, &okay)-1;
	if(!okay) { return; }
	int N = QInputDialog::getInt(this, tr("Remove Nth Row"), tr("N = ?"), 0, 0, rowCount()-1, 1, &okay);
	if(!okay) { return; }

	if(!okay)
	{
		return;
	}

	toolsSelectEveryNthSprite(offset, N);
}

char getPartNumber(char part);

void SpriteTable::toolsPruneC2Sprites(char part)
{
	part = getPartNumber(part);

	auto command = new GroupCommand();

	for(int i = 0, k = 0; i < rowCount(); ++i)
	{
		int c = i % 10;

		if(c == 8
		|| c == 9
		|| c == part
		|| c == part+4)
		{
			++k;
			continue;
		}

		for(int j = 0; j < columnCount(); ++j)
		{
			if(item(i, j))
			{
				command->push_back(new SetCommand(this, k, j));
			}
		}

		command->push_back(new DeletionCommand(this, k));
	}

	if(command->size())
	{
		command_list.push(this, command);
	}
	else
	{
		delete command;
	}
}

void SpriteTable::toolsPruneC3Sprites(char part)
{
	toolsSelectEveryNthSprite(0, getPartNumber(part));
}

void SpriteTable::customMenuRequested(QPoint pos)
{
	QModelIndex index = indexAt(pos);

	auto obj = dynamic_cast<ImageView*>(item(index.row(), index.column()));
	bool exists = obj && !obj->image.isNull();

	QMenu *menu=new QMenu(this);
	menu->addAction(QIcon::fromTheme("edit-cut"), tr("Cut"), this, SLOT(editCut()))->setEnabled(exists);
	menu->addAction(QIcon::fromTheme("edit-copy"), tr("Copy"), this, SLOT(editCopy()))->setEnabled(exists);
	menu->addAction(QIcon::fromTheme("edit-paste"), tr("Paste"), this, SLOT(editPaste()))->setEnabled(clipboardHasImage());
	menu->addAction(QIcon::fromTheme("edit-delete"), tr("Delete"), this, SLOT(editDelete()))->setEnabled(exists);
	menu->addSeparator();
	menu->addAction(QIcon::fromTheme("insert-image"), tr("Insert"), this, SLOT(menuInsert()));
	menu->addAction(QIcon::fromTheme("document-import"), tr("Import"), this, SLOT(editImport()))->setEnabled(index.isValid());
	menu->addAction(QIcon::fromTheme("document-export"), tr("Export"), this, SLOT(editExport()))->setEnabled(exists);
	menu->addSeparator();
	menu->addAction(tr("Select Row"), this, SLOT(menuSelect()))->setEnabled(index.row() != -1);
	menu->addAction(tr("Clear Row"), this, SLOT(menuClear()))->setEnabled(index.row() != -1);
	menu->addAction(tr("Delete Row"), this, SLOT(menuDelete()))->setEnabled(index.row() != -1);

	menu->popup(viewport()->mapToGlobal(pos));
}
