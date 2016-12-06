#include "imageview.h"
#include <QPainter>
#include <QPaintEvent>
#include <QTableWidget>
#include "byteswap.h"

void writeColor(uint8_t * run, QRgb color, int index)
{
	switch(index)
	{
	case 0:
		run[0] = (qRed(color) & 0xF0) | (qGreen(color) >> 4);
		run[1] = (qBlue(color) & 0xF0) | (qAlpha(color) >> 4);
		break;
	case 1:
		run[0] = (qRed(color) & 0xF0) | (qGreen(color) >> 4);
		break;
	case 2:
		run[0] = (qRed(color) & 0xE0) | ((qGreen(color) & 0xE0) >> 3) | (qBlue(color) >> 6);
		break;
	case 3:
		run[0] = qRed(color);
		run[1] = qGreen(color);
		run[2] = qBlue(color);
		run[3] = qAlpha(color);
		break;
	default:
		break;
	}
}

int runLength(int index)
{
	switch(index)
	{
	case 0:
		return 2;
	case 1:
		return 1;
	case 2:
		return 1;
	case 3:
		return 4;
	default:
		return 0;
	}
}

static inline __attribute((const)) __attribute((always_inline))
uint8_t getBlue(uint8_t rg)
{
	float red   = ((rg & 0xF0))	   / 240.0;
	float green = ((rg & 0x0F) << 4) / 240.0;
	return sqrt(1 - (red*red + green*green))*255;
}

QRgb readColor(FILE * file, int index)
{
	uint8_t b[4];
	switch(index)
	{
	case 0:
		fread(b, 1, 2, file);

		return  ((b[1] & 0x0F) << 28)
			  | ((b[0] & 0xF0) << 16)
			  | ((b[0] & 0x0F) << 12)
			  | ((b[1] & 0xF0));
		break;
	case 1:
		fread(b, 1, 1, file);

		return  0xFF000000
			 |  ((b[0] & 0xF0) << 16)
			 |  ((b[0] & 0x0F) << 12)
			 |  getBlue(b[0]);
		break;
	case 2:
		fread(b, 1, 1, file);
		return  0xFF000000
			  | ((b[0] & 0xE0) << 16)
			  | ((b[0] & 0x1C) << 11)
			  | ((b[0] & 0x03) << 6);
		break;
	default:
		fread(b, 1, 4, file);
		return ((((((int) b[3] << 8) | b[0]) << 8) | b[1]) << 8) | b[2];
	}
}


ImageView::ImageView(int row, int col) :
	row(row),
	column(col)
{
}

ImageView::ImageView(QTableWidget * table, int row, int col) :
	ImageView(row, col)
{
	initialize(table);
}

ImageView::~ImageView()
{
	deinitialize();
}


QRect calculateBoundingBox(const QImage & img)
{
	int min_x = img.width(), min_y = img.height(), max_x = 0, max_y = 0;

	for(int x = 0; x < img.width(); ++x)
	{
		for(int y = 0; y < img.height(); ++y)
		{
			if(qAlpha(img.pixel(x, y)) > 64)
			{
				if(min_x > x)
				{
					min_x = x;
				}
				if(max_x < x)
				{
					max_x = x;
				}
				if(min_y > y)
				{
					min_y = y;
				}
				if(max_y < y)
				{
					max_y = y;
				}
			}
		}
	}

	return QRect(min_x, min_y, max_x - min_x, max_y - min_y);
}

template<typename T>
static inline
T __attribute((always_inline)) sq(const T & t)
{
	return t*t;
}


void ImageView::setThumbnail()
{
	bounds = calculateBoundingBox(image);
	QImage img = image;

	for(int x = 0; x < image.width(); ++x)
	{
		for(int y = 0; y < image.height(); ++y)
		{
			auto p = image.pixel(x, y);
			int alpha = qAlpha(p);

			if(alpha == 255) continue;

			auto color = 0xFF;
			if(((x / 8 + 1) & 0x01) ^ ((y / 8 + 1) & 0x01))
			{
				color = 0xCC;
			}

			auto a = alpha/255.0;
#define blend(ca, aa, cb, ab) (ca*aa + cb*ab * (1-aa)) / (aa + ab*(1 - aa))
#if 0
			img.setPixel(x, y, qRgba(
				sqrt(blend(sq(qRed  (p)), a, sq(color), 1.0)),
				sqrt(blend(sq(qGreen(p)), a, sq(color), 1.0)),
				sqrt(blend(sq(qBlue (p)), a, sq(color), 1.0)),
				0xFF
			)
			);
#else
			img.setPixel(x, y, qRgba(
				blend(qRed  (p), a, color, 1.0),
				blend(qGreen(p), a, color, 1.0),
				blend(qBlue (p), a, color, 1.0),
				0xFF
			)
			);
#endif
#undef blend
		}
	}

	setData(Qt::DecorationRole, QPixmap::fromImage(img));
}

bool ImageView::setImage(QImage img)
{
	if(img.isNull())
	{
		original = img;
		image = img;
		bounds = QRect();
		return true;
	}

//
	original = img;

	if(column == 0)
	{
		image = img.convertToFormat(QImage::Format_ARGB4444_Premultiplied);
	}
	else if(column == 1)
	{
		for(int x = 0; x < img.width(); ++x)
		{
			for(int y = 0; y < img.height(); ++y)
			{
				auto col = img.pixel(x, y);
				if(qAlpha(col) < 128)
				{
					img.setPixel(x, y, 0);
					continue;
				}

				if(!(col & 0x00FFFFFF))
				{
					continue;
				}

				double red   = qRed(col)	/ 128.0 - 1;
				double green = qGreen(col)  / 128.0 - 1;
				double blue  = qBlue(col)	/ 128.0 - 1;

				double length = sqrt(red*red + green*green + blue*blue);
				red		/= length;
				green	/= length;
				blue	 = sqrt(1 - (red*red + green*green));

				int r = (red + 1)/2   * 255;
				int g = (green + 1)/2 * 255;
				int b =  blue * 255;

				img.setPixel(x, y, qRgba(r, g, b, 0xFF));
			}
		}

		image = img.convertToFormat(QImage::Format_ARGB4444_Premultiplied);
	}
	else if(column == 2)
	{
		static QVector<QRgb> palette332;

		if(!palette332.size())
		{
			for(int r = 0; r < 8; ++r)
			{
				for(int g = 0; g < 8; ++g)
				{
					for(int b = 0; b < 4; ++b)
					{
						palette332.push_back(qRgba(r << 5, g << 5, b << 6, 0xFF));
					}
				}
			}
		}

		QImage dithered = img.convertToFormat(QImage::Format_Indexed8, palette332, Qt::PreferDither);
		dithered = dithered.convertToFormat(QImage::Format_ARGB4444_Premultiplied);

		for(int x = 0; x < img.width(); ++x)
		{
			for(int y = 0; y < img.height(); ++y)
			{
				if(qAlpha(img.pixel(x, y)) < 128)
				{
					dithered.setPixel(x, y, 0);
					continue;
				}
			}
		}

		image = dithered;
	}
	else if(column == 3)
	{
		image = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
	}

	bounds = calculateBoundingBox(image);

	setThumbnail();

	return true;
}

void ImageView::writeImage(FILE * file, uint32_t * offset)
{
	unsigned char * run = (unsigned char *) calloc(4, image.width());

	for(int y = 0; y < image.height(); ++y)
	{
		offset[y] = byte_swap((unsigned int) ftell(file));

		for(int x = 0; x < image.width(); )
		{
// run length
			uint16_t length;

			for(length = 0; x < image.width(); ++length, ++x)
			{
				if(qAlpha(image.pixel(x, y)) != 0)
				{
					break;
				}
			}

			if(length == image.width())
			{
				offset[y] = 0;
				break;
			}

			length = byte_swap(length);
			fwrite(&length, 2, 1, file);

			auto ptr = run;

			for(length = 0; x < image.width(); ++length, ++x)
			{
				auto col = image.pixel(x, y);
				if(qAlpha(col) == 0)
				{
					break;
				}

				writeColor(ptr, col, column);
				ptr += runLength(column);
			}

			length = byte_swap(length);
			fwrite(&length, 2, 1, file);
			fwrite(run, runLength(column), length, file);
		}
	}

	free(run);
}

void ImageView::readImage(FILE * file, short w, short h)
{
	image = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
	image.fill(0);

	uint32_t * offset = (uint32_t *) malloc(sizeof(uint32_t) * h);
	fread(offset, sizeof(uint32_t), h, file);

	for(int y = 0; y < image.height(); ++y)
	{
		if(offset[y] == 0)
		{
			continue;
		}

		fseek(file, byte_swap(offset[y]), SEEK_SET);

		for(int x = 0; x < image.width(); )
		{
			uint16_t length;
			fread(&length, sizeof(uint16_t), 1, file);
			length = byte_swap(length);

			x += length;

			if(x >= image.width())
			{
				break;
			}

			fread(&length, sizeof(uint16_t), 1, file);
			length = byte_swap(length);

			for(; length > 0; --length, ++x)
			{
				image.setPixel(x, y, readColor(file, column));
			}
		}

	}

	free(offset);
	setThumbnail();
}
void ImageView::initialize(QTableWidget *table)
{
	map.reserve(table->columnCount());
	for(int i = 0; i < table->columnCount(); ++i)
	{
		if(i == column)
		{
			map.push_back(this);
		}
		else
		{
			auto widget = dynamic_cast<ImageView *>(table->cellWidget(row, i));
			map.push_back(widget);
			if(widget != 0L)
			{
				widget->map[column] = this;
			}
		}
	}
}

void ImageView::deinitialize()
{
	for(uint8_t i = 0; i < map.size(); ++i)
	{
		if(map[i])
		{
			map[i]->map[column] = 0L;
		}
	}

	map.clear();
}
