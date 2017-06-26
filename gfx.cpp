#include "stdafx.h"
#include "gfx.h"

void Convert2bpp(Image &img, int x, int y, u8 *data)
{
	for (int _y = 0; _y < 8; _y++, data += 2)
	{
		u8 b0 = data[0];
		u8 b1 = data[1];
		for (int _x = 7; _x >= 0; _x--)
		{
			img.SetPixelAt(x + _x, y + _y, (b0 & 1) | ((b1 & 1) << 1));
			b0 >>= 1;
			b1 >>= 1;
		}
	}
}

void Convert4bpp(Image &img, int x, int y, u8 *data)
{
	for (int _y = 0; _y < 8; _y++, data += 2)
	{
		u8 b0 = data[0];
		u8 b1 = data[1];
		u8 b2 = data[16];
		u8 b3 = data[17];
		for (int _x = 7; _x >= 0; _x--)
		{
			img.SetPixelAt(x + _x, y + _y, (b0 & 1) | ((b1 & 1) << 1)
				| ((b2 & 1) << 2) | ((b3 & 1) << 3));
			b0 >>= 1;
			b1 >>= 1;
			b2 >>= 1;
			b3 >>= 1;
		}
	}
}

void MakeImage2bpp(Image &img, GFX_HEADER *head, u8 *data)
{
	RGBQUAD pal[16];
	// transparency
	*(u32*)&pal[0] = RGBQ(0, 128, 192);
	// build greyscale
	for (int i = 1; i < 4; i++)
		*(u32*)&pal[i] = RGBQ(i * 64, i * 64, i * 64);

	int w = head->w, h = head->h;
	img.Create(w * 8, h * 8, 4, pal);

	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			Convert2bpp(img, j * 8, i * 8, data);
			data += 16;
		}
	}
}

void MakeImage4bpp(Image &img, GFX_HEADER *head, u8 *data)
{
	RGBQUAD pal[16];
	// transparency
	*(u32*)&pal[0] = RGBQ(0, 128, 192);
	// build greyscale
	for (int i = 1; i < 16; i++)
		*(u32*)&pal[i] = RGBQ(i * 16, i * 16, i * 16);

	int w = head->w, h = head->h;
	img.Create(w * 8, h * 8, 4, pal);

	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			Convert4bpp(img, j * 8, i * 8, data);
			data += 32;
		}
	}
}

void Write2bpp(Image &img, int x, int y, u8 *data)
{
	for (int _y = 0; _y < 8; _y++)
	{
		u8 b0 = 0, b1 = 0;
		for (int _x = 0; _x < 8; _x++)
		{
			b0 <<= 1;
			b1 <<= 1;
			u8 p = img.GetPixelAt(x + _x, y + _y) & 3;
			b0 |= p & 1;
			b1 |= (p >> 1) & 1;
		}
		*data++ = b0;
		*data++ = b1;
	}
}

void Write4bpp(Image &img, int x, int y, u8 *data)
{
	for (int _y = 0; _y < 8; _y++, data += 2)
	{
		u8 b0 = 0, b1 = 0, b2 = 0, b3 = 0;
		for (int _x = 0; _x < 8; _x++)
		{
			b0 <<= 1;
			b1 <<= 1;
			b2 <<= 1;
			b3 <<= 1;
			u8 p = img.GetPixelAt(x + _x, y + _y);
			b0 |= p & 1;
			b1 |= (p & 2) >> 1;
			b2 |= (p & 4) >> 2;
			b3 |= (p & 8) >> 3;
		}
		data[0] = b0;
		data[1] = b1;
		data[16] = b2;
		data[17] = b3;
	}
}

void ConvertImage2bpp(Image &img, GFX_HEADER *head, u8* &data, int &size)
{
	head->w = img.width / 8;
	head->h = img.height / 8;
	head->depth = 2;

	// allocate enough data for tiles
	size = head->w * head->h * 16;
	data = new u8[size];
	// quick buffer for writing
	u8 *dst = data;
	for (int i = 0, h = head->h; i < h; i++)
	{
		for (int j = 0, w = head->w; j < w; j++)
		{
			Write2bpp(img, j * 8, i * 8, dst);
			dst += 16;
		}
	}
}

void ConvertImage4bpp(Image &img, GFX_HEADER *head, u8* &data, int &size)
{
	head->w = img.width / 8;
	head->h = img.height / 8;
	head->depth = 4;

	// allocate enough data for tiles
	size = head->w * head->h * 32;
	data = new u8[size];
	// quick buffer for writing
	u8 *dst = data;
	for (int i = 0, h = head->h; i < h; i++)
	{
		for (int j = 0, w = head->w; j < w; j++)
		{
			Write4bpp(img, j * 8, i * 8, dst);
			dst += 32;
		}
	}
}
