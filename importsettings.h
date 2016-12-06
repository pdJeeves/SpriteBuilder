#ifndef IMPORTSETTINGS_H
#define IMPORTSETTINGS_H

#include <QDialog>

namespace Ui {
class ImportSettings;
}

struct import_settings
{
	import_settings();

	uint8_t import_time;
	unsigned blur_iterations : 4;
	unsigned alpha_iterations : 4;

	bool reverse_dithering : 1;
	bool resize : 1;
	bool resize_linear : 1;
	bool resize_bilinear : 1;
	bool resize_xbr : 1;

	bool eliminate_unnecessary : 1;
	bool reorder_sprites : 1;
	bool bilaterally_symmetrical : 1;
	bool eliminate_reverse_blink: 1;

	bool accepted : 1;
};


class ImportSettings : public QDialog
{
	Q_OBJECT
	import_settings & set;

public:
	explicit ImportSettings(QWidget *parent, import_settings & set);
	~ImportSettings();

private:
	Ui::ImportSettings *ui;
};

#endif // IMPORTSETTINGS_H
