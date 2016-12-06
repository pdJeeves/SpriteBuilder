#ifndef SPRITEBUILDER_H
#define SPRITEBUILDER_H

#include <QMainWindow>
#include <QTimer>

#define MARGIN_SIZE 20

namespace Ui {
class SpriteBuilder;
}

class SpriteBuilder : public QMainWindow
{
	Q_OBJECT
	QTimer autosave_timer;

	QString filename;
	bool imported;

	void _documentSave();
public:
	explicit SpriteBuilder(QWidget *parent = 0);
	~SpriteBuilder();

	void closeEvent(QCloseEvent * event);

	static bool clipboardHasImage();

public slots:
	void editUndo();
	void editRedo();

	void documentNew();
	void documentOpen();
	void documentSave();
	void documentSaveAs();
	void documentExport();
	void documentImport();
	void documentExportAll();
	bool documentClose();

	void toolsPruneC2Sprites();
	void toolsPruneC3Sprites();

	void helpAbout();
	void importC1();

	void autoSave();
	void updateTitleBar();

private:
	bool documentPreClose();
	Ui::SpriteBuilder *ui;
};

#endif // SPRITEBUILDER_H
