#include "spritebuilder.h"
#include "ui_spritebuilder.h"
#include "commandchain.h"
#include "imageview.h"
#include <QKeyEvent>
#include <QWheelEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QClipboard>
#include <QMimeData>
#include <QImageReader>
#include <QImageWriter>
#include <QStandardPaths>
#include "byteswap.h"



SpriteBuilder::SpriteBuilder(QWidget *parent) :
QMainWindow(parent),
autosave_timer(this),
ui(new Ui::SpriteBuilder)
{
	ui->setupUi(this);

	connect(ui->actionNew,		SIGNAL(triggered()), this, SLOT(documentNew()));
	connect(ui->actionOpen,		SIGNAL(triggered()), this, SLOT(documentOpen()));
	connect(ui->actionSave,		SIGNAL(triggered()), this, SLOT(documentSave()));
	connect(ui->actionSaveAs,	SIGNAL(triggered()), this, SLOT(documentSaveAs()));

	connect(ui->actionImportAll,	SIGNAL(triggered()), this, SLOT(documentImport()));
	connect(ui->actionImportC1,	SIGNAL(triggered()), this, SLOT(importC1()));
	connect(ui->actionAbout,	SIGNAL(triggered()), this, SLOT(helpAbout()));


	connect(ui->actionUndo,		SIGNAL(triggered()), this, SLOT(editUndo()));
	connect(ui->actionRedo,		SIGNAL(triggered()), this, SLOT(editRedo()));

	connect(ui->actionCut,		SIGNAL(triggered()), ui->tableWidget, SLOT(editCut()));
	connect(ui->actionCopy,		SIGNAL(triggered()), ui->tableWidget, SLOT(editCopy()));
	connect(ui->actionPaste,	SIGNAL(triggered()), ui->tableWidget, SLOT(editPaste()));
	connect(ui->actionDelete,	SIGNAL(triggered()), ui->tableWidget, SLOT(editDelete()));
	connect(ui->actionInsert,	SIGNAL(triggered()), ui->tableWidget, SLOT(menuInsert()));
	connect(ui->actionImportImage,	SIGNAL(triggered()), ui->tableWidget, SLOT(editImport()));
	connect(ui->actionExportImage,	SIGNAL(triggered()), ui->tableWidget, SLOT(editExport()));


	connect(ui->actionInterpolateColors,	SIGNAL(triggered()), ui->tableWidget, SLOT(toolsInterpolateColor()));
	connect(ui->actionBlurAlpha,	SIGNAL(triggered()), ui->tableWidget, SLOT(toolsBlurAlpha()));
	connect(ui->actionScaleImages,	SIGNAL(triggered()), ui->tableWidget, SLOT(toolsScaleImages()));

	connect(ui->actionPruneC2SpritePositions,	SIGNAL(triggered()), this, SLOT(toolsPruneC2Sprites()));
	connect(ui->actionPrune_c2e_Sprites,	SIGNAL(triggered()), this, SLOT(toolsPruneC3Sprites()));


	connect(ui->actionSelect_Nth_Image,	SIGNAL(triggered()), ui->tableWidget, SLOT(toolsSelectNthSprite()));
	connect(ui->actionSelectEveryNthImage,	SIGNAL(triggered()), ui->tableWidget, SLOT(toolsSelectEveryNthSprite()));
	connect(ui->actionClear_Row,	SIGNAL(triggered()), ui->tableWidget, SLOT(toolsClearNthRow()));
	connect(ui->actionClear_Every_Nth_Row,	SIGNAL(triggered()), ui->tableWidget, SLOT(toolsClearEveryNthRow()));
	connect(ui->actionRemove_Nth_Image,	SIGNAL(triggered()), ui->tableWidget, SLOT(toolsRemoveNthRow()));
	connect(ui->actionRemove_Every_Nth_Image,	SIGNAL(triggered()), ui->tableWidget, SLOT(toolsRemoveEveryNthRow()));

	setGeometry(
	    QStyle::alignedRect(
	        Qt::LeftToRight,
	        Qt::AlignCenter,
	        size(),
	        qApp->desktop()->availableGeometry()
	    )
	);

	ui->tableWidget->command_list.builder = this;
	imported = false;

	connect(&autosave_timer, SIGNAL(timeout()), this, SLOT(autoSave()));

	autosave_timer.setInterval(5*60*1000);
	autosave_timer.setTimerType(Qt::VeryCoarseTimer);
	autosave_timer.setSingleShot(false);
}

SpriteBuilder::~SpriteBuilder()
{
	delete ui;
}


void SpriteBuilder::closeEvent(QCloseEvent * event)
{
	if(documentClose())
	{
		QMainWindow::closeEvent(event);
	}
}

void SpriteBuilder::editUndo()
{
	ui->tableWidget->editUndo();
	updateTitleBar();
}

void SpriteBuilder::editRedo()
{
	ui->tableWidget->editRedo();
	updateTitleBar();
}


void SpriteBuilder::documentOpen()
{
	if(!documentPreClose())
	{
		return;
	}

	QString name = QFileDialog::getOpenFileName(this, tr("Open Freetures Sprite"), QString(), tr("Freetures Sprite (*.c32)"));

	if(name.isEmpty())
	{
		return;
	}


	QByteArray ba = name.toLatin1();
	const char *c_str2 = ba.data();

	FILE * file = fopen(c_str2, "r");
	if(!file)
	{
		QMessageBox mesg;
		mesg.setText(QObject::tr("Unable to open file '%1' for reading.").arg(filename));
		mesg.exec();
		documentOpen();
		return;
	}

	ui->tableWidget->clearContents();
	ui->tableWidget->setRowCount(0);
	autosave_timer.stop();
	imported = false;

	filename = name;
	ui->tableWidget->command_list.clear();

	int _one;
	fread(&_one, 4, 1, file);
	short size;
	fread(&size, sizeof(size), 1, file);
	size = byte_swap(size);

	for(int i = 0; i < size; ++i)
	{
		uint16_t width, height;
		fread(&width, sizeof(uint16_t), 1, file);
		fread(&height, sizeof(uint16_t), 1, file);
		width = byte_swap(width);
		height = byte_swap(height);

		ui->tableWidget->insertRow(i);

		for(int j = 0; j < ui->tableWidget->columnCount(); ++j)
		{
			uint32_t offset;
			fread(&offset, sizeof(uint32_t), 1, file);
			offset = byte_swap(offset);

			if(!offset)
			{
				continue;
			}

			fpos_t pos;
			fgetpos(file, &pos);

			auto image = new ImageView(ui->tableWidget, i, j);

			fseek(file, offset, SEEK_SET);

			image->readImage(file, width, height);

			ui->tableWidget->setItem(i, j, image);

			fsetpos(file, &pos);
		}

	}

	fclose(file);

	ui->tableWidget->resizeColumnsToContents();
	ui->tableWidget->resizeRowsToContents();

	autosave_timer.start();
	updateTitleBar();
}

struct image_header
{
	uint16_t width, height;
	uint32_t offset[4];
};

void SpriteBuilder::_documentSave()
{
	if(filename.isEmpty())
	{
		documentSaveAs();
		return;
	}

	QByteArray ba = filename.toLatin1();
	const char *c_str2 = ba.data();

	FILE * file = fopen(c_str2, "w");
	if(!file)
	{
		QMessageBox mesg;
		mesg.setText(QObject::tr("Unable to open file '%1' for writing.").arg(filename));
		mesg.exec();
		filename = QString();
		documentSaveAs();
		return;
	}

	ui->tableWidget->command_list.onSave();

	int _one = byte_swap((unsigned int) 1);

	fwrite(&_one, 4, 1, file);
	short size = byte_swap(ui->tableWidget->rowCount());
	fwrite(&size, sizeof(size), 1, file);

	fpos_t header_pos;
	fgetpos(file, &header_pos);
	image_header * header = (image_header *) calloc(sizeof(image_header), size);

	int total_offset_length = 0;
	auto offset = ftell(file) + sizeof(image_header) * size;

	for(int i = 0; i < ui->tableWidget->rowCount(); ++i)
	{
		bool saved_metadata = false;

		for(int j = 0; j < 4; ++j)
		{
			auto image = dynamic_cast<ImageView*>(ui->tableWidget->item(i, j));

			if(!image || image->image.isNull())
			{
				continue;
			}

			if(!saved_metadata)
			{
				saved_metadata      = true;
				header[i].width     = byte_swap((uint16_t) image->image.width());
				header[i].height    = byte_swap((uint16_t) image->image.height());
			}

			header[i].offset[j] = byte_swap((uint32_t) (offset + total_offset_length * 4));
			total_offset_length += image->image.height();
		}
	}

	fwrite(header, sizeof(image_header), size, file);
	free(header);

	uint32_t * row_offsets = (uint32_t *) calloc(sizeof(uint32_t), total_offset_length);

	fpos_t offset_pos;
	fgetpos(file, &offset_pos);

	fseek(file, 4 * total_offset_length, SEEK_CUR);

	total_offset_length = 0;
	for(int i = 0; i < ui->tableWidget->rowCount(); ++i)
	{
		for(int j = 0; j < 4; ++j)
		{
			auto image = dynamic_cast<ImageView*>(ui->tableWidget->item(i, j));

			if(!image || image->image.isNull())
			{
				continue;
			}

			image->writeImage(file, row_offsets + total_offset_length);
			total_offset_length += image->image.height();
		}
	}

	fsetpos(file, &offset_pos);
	fwrite(row_offsets, sizeof(uint32_t), total_offset_length, file);
	fclose(file);
	updateTitleBar();
	autosave_timer.start();
}


void SpriteBuilder::documentSave()
{
	if(!ui->tableWidget->command_list.dirtyPage())
	{
		return;
	}

	_documentSave();
}

void SpriteBuilder::documentSaveAs()
{
	QString name = QFileDialog::getSaveFileName(this, tr("Export Sprite"), QString(), tr("Freetures Sprite (*.c32)"));

	if(name.isEmpty())
	{
		return;
	}

	filename = name;

	_documentSave();
}

void SpriteBuilder::helpAbout()
{
	QApplication::aboutQt();
}

void SpriteBuilder::documentExport()
{

}

void SpriteBuilder::documentNew() { documentClose(); }


void SpriteBuilder::documentExportAll() { }


bool SpriteBuilder::documentPreClose()
{
	if(ui->tableWidget->command_list.dirtyPage())
	{
		auto name = filename.isEmpty()? tr("untitled") : filename;

		auto result = QMessageBox::question(this, QGuiApplication::applicationDisplayName(), tr("Do you want to save changes to '%1'?").arg(name),
						(QMessageBox::Save | QMessageBox::Cancel | QMessageBox::Discard), QMessageBox::Save);

		if(result == QMessageBox::Save)
		{
			documentSave();
		}
		else if(result == QMessageBox::Cancel)
		{
			return false;
		}
	}

	return true;
}


bool SpriteBuilder::documentClose()
{
	if(documentPreClose())
	{
		ui->tableWidget->clearContents();
		ui->tableWidget->setRowCount(0);
		autosave_timer.stop();
		imported = false;
		filename = QString();
		return true;
	}

	return false;
}

void SpriteBuilder::toolsPruneC2Sprites()
{
	QStringList parts = filename.split("/");
	ui->tableWidget->toolsPruneC2Sprites(parts.at(parts.size()-1)[0].toLatin1());
}

void SpriteBuilder::toolsPruneC3Sprites()
{
	QStringList parts = filename.split("/");
	ui->tableWidget->toolsPruneC3Sprites(parts.at(parts.size()-1)[0].toLatin1());
}

void SpriteBuilder::autoSave()
{
	if(filename.isEmpty() || imported)
	{
		return;
	}

	documentSave();
}

void SpriteBuilder::updateTitleBar()
{
	QString title;

	if(!filename.isEmpty())
	{
		QString title = tr("%1 -").arg(QFileInfo(filename ).fileName());
	}

	title = tr("%1%2").arg(title, QFileInfo( QCoreApplication::applicationFilePath() ).fileName());

	if(ui->tableWidget->command_list.dirtyPage())
	{
		setWindowTitle(QString("*").append(title));
	}
	else
	{
		setWindowTitle(title);
	}

	ui->actionUndo->setEnabled(ui->tableWidget->command_list.canRollBack());
	ui->actionRedo->setEnabled(ui->tableWidget->command_list.canRollForward());
}
