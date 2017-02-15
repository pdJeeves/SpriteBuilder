#include "imageview.h"
#include <QPainter>
#include <QPaintEvent>
#include <QTableWidget>
#include "byteswap.h"
#include <iostream>
#include <squish.h>

uint32_t packBytes(uint32_t a, uint32_t b, uint32_t c,uint32_t d)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return (((((d << 8) | c ) << 8) | b) << 8) | a;
#else
	return (((((a << 8) | b ) << 8) | c) << 8) | d;
#endif
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
T ALWAYS_INLINE sq(const T & t)
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
		image = img;
		bounds = QRect();
		return true;
	}

	image = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
	bounds = calculateBoundingBox(image);

	setThumbnail();

	return true;
}


uint32_t ImageView::getRunLength(int i, bool transparent)
{
	uint32_t size = image.width() * image.height();

	for(uint32_t j = i; j < size; ++j)
	{
		int x = j % image.width();
		int y = j / image.width();

		if(((bool) qAlpha(image.pixel(x, y))) == transparent)
		{
			return j - i;
		}
	}

	return size - i;
}

struct BLOCK_128
{
	uint8_t data[16];

	operator bool() const
	{
		return
		(data[0] | (data[1] & ~0x05) | data[2] | data[3] |
		 data[4] | data[5] | data[6] | data[7] |
		 data[8] | data[9] | data[10] | data[11] |
		 data[12] | data[13] | data[14] | data[15]);
	}
};

struct BLOCK_64
{
	uint8_t data[8];

	operator bool() const
	{
#if 1
		static uint8_t cmp[] = { 0, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF };
		auto r = memcmp(data, cmp, 8);
		return r;
#else
		return true;
	#endif
	}
};

void writeDtx1(FILE * file, std::vector<uint32_t> & uncompressed_image, int width, int height)
{
	uint32_t size = GetStorageRequirements(width, height, squish::kDxt1);

	{
		uint8_t type = 1;
		fwrite(&type, 1, 1, file);
		fwrite(&size, 4, 1, file);
	}

	std::vector<BLOCK_64> blocks(size >> 3);
	squish::CompressImage((uint8_t*) uncompressed_image.data(), width, height, (void*) blocks.data(),
		squish::kDxt1);
#if 0
	uint32_t length = 0;
	fwrite(&length, 4, 1, file);
	fwrite(&size, 4, 1, file);
	fwrite(blocks.data(), 8, blocks.size(), file);
#else
	for(size_t i = 0; i < blocks.size(); )
	{
		uint32_t length;
		for(length = 0; i < blocks.size() && !blocks[i]; ++i, ++length) {}

		length <<= 3;
		length = byte_swap(length);
		fwrite(&length, 4, 1, file);

		if(i >= size)
		{
			break;
		}

		for(length = 0; i+length < blocks.size() && blocks[i+length]; ++length)  {}

		{
			uint32_t len = length << 3;
			len = byte_swap(len);
			fwrite(&len, 4, 1, file);
		}

		fwrite(blocks.data() + i, 8, length, file);
		i += length;
	}
#endif
}

uint8_t scanAlpha(const QImage & image, const uint16_t x, const uint16_t y)
{
	uint8_t min = 255;
	uint8_t max = 0;

	for(uint16_t _y = y; _y < y+4; ++_y)
	{
		for(uint16_t _x = x; _x < x+4; ++_x)
		{
			uint8_t c = qAlpha(image.pixel(_x, _y));

			min = std::min(min, c);
			max = std::max(max, c);
		}
	}

	if(((min >> 4) == 0 || (max >> 4) == 0x0F))
	{
		return 1;
	}

	if(0x01 > (max - min))
	{
		return 5;
	}

	return 3;
}

void ImageView::writeImage(FILE * file)
{
	uint32_t size = image.width()*image.height();

	std::vector<uint32_t> uncompressed_image(size, 0);

	uint16_t compression_1 = 0;
	uint16_t compression_3 = 0;
	uint16_t compression_5 = 0;

	for(int y = 0; y < image.height(); y += 4)
	{
		for(int x = 0; x < image.width(); x += 4)
		{
			switch(scanAlpha(image, x, y))
			{
			case 1:
				++compression_1;
				break;
			case 3:
				++compression_3;
				break;
			case 5:
				++compression_5;
				break;
			}
		}
	}

	uint8_t compression_type = 1;

	if(compression_3 > 8 && compression_3 > compression_5)
	{
		compression_type = 3;
	}
	else if(compression_5 > 8 && compression_5 >= sqrt(compression_3)/2)
	{
		compression_type = 5;
	}

	for(uint32_t i = 0; i < size; ++i)
	{
		uint16_t x = i % image.width();
		uint16_t y = i / image.width();

		QRgb c = image.pixel(x, y);

		if(!qAlpha(c))
		{
			continue;
		}

		if(column >= 2)
		{
			c = (uint32_t) packBytes(0, qGreen(c), 0, qRed(c));
			continue;
		}

		uncompressed_image[i] = (uint32_t) packBytes(qRed(c), qGreen(c), qBlue(c), qAlpha(c));
	}


	if(compression_type == 1)
	{
		writeDtx1(file, uncompressed_image, image.width(), image.height());
		return;
	}

	fwrite(&compression_type, 1, 1, file);
	switch(compression_type)
	{
	default:
		compression_type = squish::kDxt3;
		break;
	case 5:
		compression_type = squish::kDxt5;
		break;
	}

	size = squish::GetStorageRequirements( image.width(), image.height(), compression_type);
	{
		uint32_t length = byte_swap(size);
		fwrite(&length, 4, 1, file);
	}

	std::vector<BLOCK_128> blocks(size);
	squish::CompressImage((uint8_t*) uncompressed_image.data(), image.width(), image.height(), (void*) blocks.data(),
		compression_type | squish::kColourIterativeClusterFit | squish::kWeightColourByAlpha );

	for(size_t i = 0; i < size; )
	{
		uint32_t length;
		for(length = 0; i < size && !blocks[i]; ++i, ++length) {}

		length <<= 4;
		length = byte_swap(length);
		fwrite(&length, 4, 1, file);

		if(i >= size)
		{
			break;
		}

		for(length = 0; i+length < size && blocks[i+length]; ++length)  {}

		{
			uint32_t len = length << 4;
			len = byte_swap(len);
			fwrite(&len, 4, 1, file);
		}

		fwrite(blocks.data() + i, 16, length, file);
		i += length;
	}
}

void ImageView::readImage(FILE * file, short w, short h)
{
	uint8_t compression_type;
	fread(&compression_type, 1, 1, file);
	switch(compression_type)
	{
	default:
		compression_type = squish::kDxt1;
		break;
	case 3:
		compression_type = squish::kDxt3;
		break;
	case 5:
		compression_type = squish::kDxt5;
		break;
	}

	uint32_t size;
	fread(&size, 4, 1, file);
	size = byte_swap(size);

	std::vector<uint8_t> compressed_image(size, 0);

	for(int i = 0; i < size; )
	{
		uint32_t length;
		fread(&length, sizeof(length), 1, file);
		length = byte_swap(length);

		switch(compression_type)
		{
		case 1:
			for(; i+8 <= size && length; length -= 8, i += 8)
			{
				memset(compressed_image.data() + (i+4), 0xFF, 4);
			}
			break;
		case 3:
			break;
		case 5:
			for(; i+16 <= size && length; length -= 16, i += 16)
			{
				compressed_image[i+1] = 0x05;
			}
			break;
		}

		i += length;

		if(i >= size)
		{
			break;
		}

		fread(&length, 4, 1, file);
		length = byte_swap(length);
		fread(compressed_image.data() + i, 1, length, file);
		i += length;
	}

	size = w*h;
	std::vector<QRgb> uncompressed_image(size);
	squish::DecompressImage((uint8_t*) uncompressed_image.data(), w, h, compressed_image.data(), compression_type);
	compressed_image.clear();

	image = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
	image.fill(0);

	for(int i = 0; i < size; ++i)
	{
		QRgb c = uncompressed_image[i];
		if(c)
		{
			c = packBytes(qRed(c), qGreen(c), qBlue(c), qAlpha(c));

			if(column >= 2)
			{
				c = qRgba(qBlue(c), qRed(c), 0, 255);
			}
		}

		image.setPixel(i % w, i / w, c);
	}

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
