#include "spritetable.h"
#include "spritebuilder.h"
#include "imageview.h"
#include <QApplication>
#include <QImageReader>
#include <QClipboard>
#include <QMessageBox>
#include <QDir>
#include <QFileDialog>
#include <QMimeData>
#include <QStandardPaths>
#include <QImageWriter>


QImage SpriteTable::imageFromClipboard()
{
	const QClipboard *clipboard = QApplication::clipboard();
	return imageFromeMime(const_cast<QMimeData*>(clipboard->mimeData()));
}

bool SpriteTable::loadImage(const QString &fileName, QImage & newImage)
{
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    newImage = reader.read();

    if (newImage.isNull()) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot load %1: %2")
                                 .arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }

	return true;
}

bool SpriteTable::saveImage(const QString &fileName, const QImage & image)
{
    QImageWriter writer(fileName);

    if (!writer.canWrite()) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot save %1: %2")
                                 .arg(QDir::toNativeSeparators(fileName), writer.errorString()));
        return false;
    }

	writer.write(image);

	if (writer.error()) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot save %1: %2")
                                 .arg(QDir::toNativeSeparators(fileName), writer.errorString()));
        return false;
    }


	return true;
}

void initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode, QFileDialog::FileMode mode = QFileDialog::ExistingFile)
{
    static bool firstDialog = true;

    if (firstDialog) {
        firstDialog = false;
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
    }

	dialog.setFileMode(mode);

    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
        ? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
    foreach (const QByteArray &mimeTypeName, supportedMimeTypes)
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");
    if (acceptMode == QFileDialog::AcceptSave)
        dialog.setDefaultSuffix("jpg");
}

QImage SpriteTable::openImage()
{
    QFileDialog dialog(this, tr("Import Image File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);

	QImage image;

    while (dialog.exec() == QDialog::Accepted && !loadImage(dialog.selectedFiles().first(), image)) {}

	return image;
}



bool SpriteTable::clipboardHasImage()
{
	const QClipboard *clipboard = QApplication::clipboard();
	const QMimeData *mimeData = clipboard->mimeData();

	if (!mimeData->hasImage()) {
		return false;
	}

	return true;
}



void SpriteTable::editUndo()
{
	if(command_list.canRollBack())
	{
		command_list.rollBack(this);
		resizeColumnsToContents();
		resizeRowsToContents();
	}
}

void SpriteTable::editRedo()
{
	if(command_list.canRollForward())
	{
		command_list.rollForward(this);
		resizeColumnsToContents();
		resizeRowsToContents();
	}
}

bool SpriteTable::editCopy()
{
	if(currentRow() == -1 || currentColumn() == -1)
	{
		return false;
	}

	ImageView * img = dynamic_cast<ImageView*>(item(currentRow(), currentColumn()));

	if(!img)
	{
		return false;
	}

	if(img->image.isNull())
	{
		return false;
	}

	QApplication::clipboard()->setImage(img->image, QClipboard::Clipboard);

	return true;
}

bool SpriteTable::editPaste()
{
	auto image = imageFromClipboard();
	if(image.isNull())
	{
		return false;
	}

	editReplaceImage(image);
	return true;
}

bool SpriteTable::editDelete()
{
	editDelete(currentRow(), currentColumn());
	return true;
}

bool SpriteTable::editImport()
{
	auto image = openImage();
	if(image.isNull())
	{
		return false;
	}

	editReplaceImage(image);
	return true;
}

bool SpriteTable::editExport()
{
	ImageView * img = dynamic_cast<ImageView*>(item(currentRow(), currentColumn()));
	if(!img || img->image.isNull())
	{
		return false;
	}

	QFileDialog dialog(this, tr("Export Image File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptSave);

	QImage image;

    while (dialog.exec() == QDialog::Accepted && !saveImage(dialog.selectedFiles().first(), image)) {}

	return true;
}

void SpriteTable::menuInsert()
{
	int row = currentRow();

	if(row == -1)
	{
		row = 0;
	}

	command_list.push(this, new InsertionCommand(this, row));
}

void SpriteTable::menuSelect()
{
	int N = currentRow();

	if(N == -1)
	{
		return;
	}

	auto command = new GroupCommand();

	for(int i = 0; i < rowCount(); ++i)
	{
		if(i == N)
		{
			continue;
		}

		for(int j = 0; j < columnCount(); ++j)
		{
			if(item(i, j))
			{
				command->push_back(new SetCommand(this, i > N, j));
			}
		}

		command->push_back(new DeletionCommand(this, i > N));
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

void SpriteTable::menuClear()
{
	int N = currentRow();

	if(N == -1)
	{
		return;
	}

	toolsClearRow(N);
}

void SpriteTable::menuDelete()
{
	int row = currentRow();

	if(row == -1)
	{
		return;
	}

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
		command->push_back(new DeletionCommand(this, row));
		command_list.push(this, command);
	}
	else
	{
		delete command;
		command_list.push(this, new DeletionCommand(this, row));
	}
}
