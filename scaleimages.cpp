#include <QImage>


void scaleSuperXbr(uint32_t * data, uint32_t * out, int w, int h);


static
void xBrLoop(std::vector<uint32_t> & pixel_data, int & width, int & height)
{
	std::vector<uint32_t> output;
	output.resize((width*2)*(height*2));

	scaleSuperXbr(pixel_data.data(), output.data(), width, height);

	width *= 2;
	height *= 2;
	pixel_data = output;
}

static
void xBrLoop(std::vector<uint32_t> & pixel_data, int & width, int & height, double scaling_factor)
{
	for(; scaling_factor > 1.0; scaling_factor /= 2)
	{
		xBrLoop(pixel_data, width, height);
	}
}


static
QImage xBr(const QImage & image, double scaling_factor)
{
	if(scaling_factor <= 1.0)
	{
		return image;
	}

	int width = image.width();
	int height = image.height();

	std::vector<uint32_t> pixel_data;
	pixel_data.resize(width*height);

	int i = 0;
	for(int y = 0; y < height; ++y)
	{
		for(int x = 0; x < width; ++x, ++i)
		{
			pixel_data[i] = image.pixel(x, y);
		}
	}

	xBrLoop(pixel_data, width, height, scaling_factor);

	QImage retn(width, height, QImage::Format_ARGB32_Premultiplied);
	retn.fill(0);

	i = 0;
	for(int y = 0; y < height; ++y)
	{
		for(int x = 0; x < width; ++x, ++i)
		{
			retn.setPixel(x, y, pixel_data[i]);
		}
	}

	return retn;
}

QImage scale_image(QImage image, double ScalingFactor)
{
	int width = image.width() * ScalingFactor + .5;
	int height = image.height() * ScalingFactor + .5;
	image = xBr(image, ScalingFactor); // */
	return image.scaled(width, height,  Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QImage double_image(QImage image)
{
	int width = image.width();
	int height = image.height();

	std::vector<uint32_t> pixel_data;
	pixel_data.resize(width*height);

	int i = 0;
	for(int y = 0; y < height; ++y)
	{
		for(int x = 0; x < width; ++x, ++i)
		{
			pixel_data[i] = image.pixel(x, y);
		}
	}

	xBrLoop(pixel_data, width, height);

	QImage retn(width, height, QImage::Format_ARGB32_Premultiplied);
	retn.fill(0);

	i = 0;
	for(int y = 0; y < height; ++y)
	{
		for(int x = 0; x < width; ++x, ++i)
		{
			retn.setPixel(x, y, pixel_data[i]);
		}
	}

	return retn;
}
