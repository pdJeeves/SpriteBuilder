#include "importsettings.h"
#include "ui_importsettings.h"

import_settings::import_settings()
{
	memset(this, 0, sizeof(import_settings));
	{
	ImportSettings dialog(0L, *this);
	accepted = dialog.exec();
	}

	import_time = 1;
}

ImportSettings::ImportSettings(QWidget *parent, import_settings & set) :
QDialog(parent),
set(set),
ui(new Ui::ImportSettings)
{
	ui->setupUi(this);
}

ImportSettings::~ImportSettings()
{
	set.blur_iterations		= ui->colorSlider->value();
	set.alpha_iterations	= ui->alphaSlider->value();
	set.reverse_dithering	= ui->Dithering->isChecked();
	set.resize				= ui->Resize->isChecked();
	set.resize_linear		= ui->nearest->isChecked();
	set.resize_bilinear		= ui->Bilinear->isChecked();
	set.resize_xbr			= ui->SuperXbr->isChecked();

	set.eliminate_unnecessary	= ui->eliminate->isChecked();
	set.reorder_sprites			= ui->reorder->isChecked();
	set.bilaterally_symmetrical	= ui->bilateral->isChecked();
	set.eliminate_reverse_blink = ui->blinking->isChecked();
	delete ui;
}
