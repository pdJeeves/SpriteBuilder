#include "spritebuilder.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	SpriteBuilder w;
	w.show();

	return a.exec();
}
